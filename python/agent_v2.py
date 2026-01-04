#!/usr/bin/env python3
"""
Agente de Terminal v2.0 com SKP (Semantic Key Protocol)
Sistema modular para análise contextual de fotos usando CLIP + LLM
"""

import os
import sys
import json
import subprocess
import platform
import argparse
import traceback
import requests
import hashlib
import re
import base64
import numpy as np
from pathlib import Path
from io import BytesIO
from typing import List, Dict, Any, Optional, Callable, Tuple
from datetime import datetime, timedelta
from dataclasses import dataclass, asdict, field
from enum import Enum
from collections import defaultdict

# Registra plugin HEIF para PIL
try:
    from pillow_heif import register_heif_opener
    register_heif_opener()
except ImportError:
    pass

# Face recognition
try:
    import cv2
    FACE_RECOGNITION_AVAILABLE = True
except ImportError:
    FACE_RECOGNITION_AVAILABLE = False

# CLIP para primeira análise
try:
    import torch
    from PIL import Image
    import clip
    CLIP_AVAILABLE = True
except ImportError:
    CLIP_AVAILABLE = False
    print("⚠️ CLIP não disponível. Instale com: pip install torch clip")

# PyIQA para análise estética
try:
    import pyiqa
    PYIQA_AVAILABLE = True
except ImportError as e:
    PYIQA_AVAILABLE = False
    print(f"⚠️ PyIQA não disponível: {e}")

# Semantic embeddings
try:
    from sentence_transformers import SentenceTransformer
    SEMANTIC_EMBEDDINGS_AVAILABLE = True
    _embedding_model = None
    def get_embedding_model():
        global _embedding_model
        if _embedding_model is None:
            _embedding_model = SentenceTransformer('all-MiniLM-L6-v2')
        return _embedding_model
except ImportError:
    SEMANTIC_EMBEDDINGS_AVAILABLE = False
    def get_embedding_model():
        return None


# ============================================================================
# SKP PROTOCOL IMPLEMENTATION
# ============================================================================

@dataclass
class SemanticKey:
    """
    Semantic Key (SK) - Chave semântica baseada em embedding
    
    Representa um campo de sentido estável que pode ser:
    - Anchor: identidade/contexto (pessoa, lugar, evento)
    - Gate: filtro/modulador (modo, atmosfera)
    - Link: vínculo entre chaves (grupo, relação)
    """
    vector: np.ndarray  # Embedding normalizado (||v|| = 1)
    key_id: str  # ID derivado (hash curto)
    role: str  # "anchor", "gate", "link"
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __post_init__(self):
        """Garante normalização"""
        norm = np.linalg.norm(self.vector)
        if norm > 0:
            self.vector = self.vector / norm
    
    def alignment(self, atom_embedding: np.ndarray) -> float:
        """
        Calcula alinhamento A(k, a) via similaridade cosseno
        Returns: valor em [0, 1]
        """
        # Normaliza atom embedding
        norm = np.linalg.norm(atom_embedding)
        if norm == 0:
            return 0.0
        atom_normalized = atom_embedding / norm
        
        # Similaridade cosseno: k · a / (||k|| ||a||)
        # Como ambos estão normalizados: k · a
        similarity = np.dot(self.vector, atom_normalized)
        
        # Converte de [-1, 1] para [0, 1]
        return (similarity + 1.0) / 2.0
    
    @staticmethod
    def compose(keys: List['SemanticKey'], weights: Optional[List[float]] = None) -> 'SemanticKey':
        """
        Composição de chaves: C(k1, k2, ..., kn, w1, w2, ..., wn)
        Retorna nova chave normalizada
        """
        if not keys:
            raise ValueError("Precisa de ao menos uma chave")
        
        if weights is None:
            weights = [1.0 / len(keys)] * len(keys)
        
        # Normaliza pesos
        total = sum(weights)
        weights = [w / total for w in weights]
        
        # Mistura ponderada
        composed = np.zeros_like(keys[0].vector)
        for key, weight in zip(keys, weights):
            composed += weight * key.vector
        
        # Normaliza resultado
        norm = np.linalg.norm(composed)
        if norm > 0:
            composed = composed / norm
        
        # Gera ID composto
        combined_ids = "_".join([k.key_id for k in keys])
        new_id = hashlib.md5(combined_ids.encode()).hexdigest()[:8]
        
        return SemanticKey(
            vector=composed,
            key_id=new_id,
            role="composite",
            metadata={
                "composed_from": [k.key_id for k in keys],
                "weights": weights
            }
        )


@dataclass
class SemanticAtom:
    """
    Semantic Atom (SA) - Átomo semântico
    
    Unidade mínima de conteúdo contextualizado:
    - data: dados brutos (filepath, pixels, etc)
    - embedding: vetor local normalizado
    """
    data: Dict[str, Any]  # Dados brutos
    embedding: np.ndarray  # Embedding local normalizado
    
    def __post_init__(self):
        """Garante normalização"""
        norm = np.linalg.norm(self.embedding)
        if norm > 0:
            self.embedding = self.embedding / norm


@dataclass
class SemanticField:
    """
    Semantic Field (F) - Campo semântico
    
    Conjunto emergente de átomos ativados por chave(s).
    Não é armazenado, é calculado dinamicamente.
    """
    keys: List[SemanticKey]
    atoms: List[SemanticAtom]
    threshold: float = 0.5
    
    def activate(self, decay_fn: Optional[Callable] = None) -> List[Tuple[SemanticAtom, float]]:
        """
        Ativa campo e retorna átomos alinhados
        Returns: Lista de (atom, alignment_score)
        """
        results = []
        
        for atom in self.atoms:
            # Calcula alinhamento agregado
            alignments = [key.alignment(atom.embedding) for key in self.keys]
            
            # Agregação: média ponderada (pode ser customizada)
            agg_alignment = sum(alignments) / len(alignments) if alignments else 0.0
            
            # Aplica decay se fornecido
            if decay_fn and 'timestamp' in atom.data:
                agg_alignment *= decay_fn(atom.data['timestamp'])
            
            if agg_alignment >= self.threshold:
                results.append((atom, agg_alignment))
        
        # Ordena por alinhamento (descendente)
        results.sort(key=lambda x: x[1], reverse=True)
        return results


# ============================================================================
# PHOTO METADATA WITH SKP
# ============================================================================

@dataclass
class PhotoMetadata:
    """Metadados de foto com suporte SKP"""
    filepath: str
    filename: str
    
    # Metadados EXIF originais
    datetime_original: Optional[datetime] = None
    camera_make: Optional[str] = None
    camera_model: Optional[str] = None
    gps_lat: Optional[float] = None
    gps_lon: Optional[float] = None
    location_name: Optional[str] = None
    
    # Face detection
    face_count: Optional[int] = None
    face_locations: Optional[List[Tuple[int, int, int, int]]] = None
    
    # Pass 1: CLIP analysis (rápida, sem LLM)
    clip_embedding: Optional[np.ndarray] = None
    clip_features: Optional[List[str]] = None  # Features detectadas pelo CLIP
    
    # Contexto de agrupamento
    group_id: Optional[str] = None
    group_context: Optional[Dict[str, Any]] = None
    
    # Pass 2: LLM analysis (contextualizada)
    llm_title: Optional[str] = None
    llm_description: Optional[str] = None
    llm_keywords: Optional[List[str]] = None
    llm_category: Optional[str] = None
    llm_scene: Optional[str] = None
    llm_mood: Optional[str] = None
    
    # SKP: Semantic Keys
    skp_image: Optional[SemanticKey] = None  # Chave da imagem
    skp_person_keys: Optional[List[SemanticKey]] = None  # Chaves de pessoas
    skp_group_keys: Optional[List[str]] = None  # IDs de chaves de grupo
    skp_global_key: Optional[str] = None  # ID da chave global do acervo
    
    # Technical analysis (Pass 1) - Tags que vão na foto
    tech_duplicate_group: Optional[str] = None  # "dup_abc123"
    tech_is_duplicate: bool = False
    tech_sharpness_score: float = 0.0  # 0-1
    tech_blur_detected: bool = False
    tech_overall_quality: float = 0.0  # Score composto
    tech_burst_group: Optional[str] = None  # "burst_xyz789"
    tech_burst_position: Optional[int] = None
    tech_is_best_in_burst: bool = False
    tech_face_quality_score: Optional[float] = None
    tech_face_eyes_open: Optional[bool] = None
    tech_face_smiling: Optional[bool] = None
    tech_exposure_quality: float = 0.0
    tech_highlights_clipped: bool = False
    tech_shadows_blocked: bool = False
    
    # Aesthetic quality (PyIQA)
    aesthetic_score: float = 0.0  # 0-1, from MUSIQ-AVA model


@dataclass
class PhotoGroup:
    """Grupo de fotos relacionadas por contexto"""
    group_id: str
    photos: List[PhotoMetadata]
    
    # Características do grupo
    start_time: Optional[datetime] = None
    end_time: Optional[datetime] = None
    location: Optional[str] = None
    location_coords: Optional[Tuple[float, float]] = None
    
    # Análise contextual (via LLM)
    llm_summary: Optional[str] = None  # Resumo do grupo
    llm_event_type: Optional[str] = None  # Tipo de evento
    llm_common_themes: Optional[List[str]] = None
    
    # SKP: Semantic Key do grupo
    skp_group_key: Optional[SemanticKey] = None
    
    def duration_minutes(self) -> float:
        """Duração do evento em minutos"""
        if self.start_time and self.end_time:
            return (self.end_time - self.start_time).total_seconds() / 60
        return 0


@dataclass
class ContextualConclusion:
    """Conclusão contextual do acervo completo"""
    total_photos: int
    groups_count: int
    
    # Análise geral (via LLM)
    llm_overall_summary: str
    llm_main_themes: List[str]
    llm_time_periods: List[str]
    llm_main_locations: List[str]
    llm_social_patterns: List[str]
    
    # SKP: Semantic Key global
    skp_global_key: SemanticKey


# ============================================================================
# CLIP ANALYZER
# ============================================================================

class CLIPAnalyzer:
    """Analisador de imagens usando CLIP (primeira passada)"""
    
    def __init__(self):
        if not CLIP_AVAILABLE:
            raise RuntimeError("CLIP não está disponível")
        
        self.device = "cuda" if torch.cuda.is_available() else "cpu"
        self.model, self.preprocess = clip.load("ViT-B/32", device=self.device)
        
        # Labels para detecção rápida
        self.labels = [
            "person", "people", "group", "family", "portrait",
            "landscape", "nature", "mountain", "beach", "ocean", "forest", "sunset",
            "city", "building", "architecture", "urban", "street",
            "food", "meal", "restaurant", "cooking",
            "indoor", "outdoor", "day", "night",
            "work", "office", "meeting", "computer",
            "travel", "vacation", "tourism", "adventure",
            "celebration", "party", "wedding", "birthday",
            "sport", "fitness", "exercise",
            "art", "museum", "gallery",
            "pet", "dog", "cat", "animal"
        ]
        
        # Pre-computa embeddings dos labels
        text_tokens = clip.tokenize(self.labels).to(self.device)
        with torch.no_grad():
            self.text_features = self.model.encode_text(text_tokens)
            self.text_features /= self.text_features.norm(dim=-1, keepdim=True)
    
    def analyze(self, image_path: str) -> Dict[str, Any]:
        """
        Analisa imagem com CLIP
        
        Returns:
            Dict com:
            - embedding: vetor normalizado (512 dims)
            - features: top N labels detectados
            - scores: scores de cada label
        """
        try:
            # Carrega imagem
            image = Image.open(image_path).convert("RGB")
            image_input = self.preprocess(image).unsqueeze(0).to(self.device)
            
            # Extrai features
            with torch.no_grad():
                image_features = self.model.encode_image(image_input)
                image_features /= image_features.norm(dim=-1, keepdim=True)
                
                # Calcula similaridade com labels
                similarity = (100.0 * image_features @ self.text_features.T).softmax(dim=-1)
                
                # Converte para numpy
                embedding = image_features.cpu().numpy()[0]
                scores = similarity.cpu().numpy()[0]
            
            # Top features
            top_indices = np.argsort(scores)[-10:][::-1]
            top_features = [(self.labels[i], float(scores[i])) for i in top_indices if scores[i] > 0.1]
            
            return {
                "embedding": embedding,
                "features": [f[0] for f in top_features[:5]],
                "scores": {f[0]: f[1] for f in top_features}
            }
            
        except Exception as e:
            Logger.error(f"Erro ao analisar com CLIP: {e}")
            return {
                "embedding": np.zeros(512),
                "features": [],
                "scores": {}
            }


# ============================================================================
# TECHNICAL IMAGE ANALYSIS (Pass 1)
# ============================================================================

class TechnicalImageAnalyzer:
    """Análises técnicas de qualidade, duplicatas, bursts"""
    
    # Class-level aesthetic metric (lazy initialization)
    _aesthetic_metric = None
    
    @classmethod
    def get_aesthetic_metric(cls):
        """Lazy initialization of PyIQA aesthetic metric"""
        if cls._aesthetic_metric is None and PYIQA_AVAILABLE:
            try:
                # Try MPS (Apple Silicon) first, then CUDA, then CPU
                if torch.backends.mps.is_available():
                    device = 'mps'
                    Logger.info("🎨 Initializing aesthetic model (MUSIQ-AVA) on Apple Silicon GPU...")
                elif torch.cuda.is_available():
                    device = 'cuda'
                    Logger.info("🎨 Initializing aesthetic model (MUSIQ-AVA) on CUDA...")
                else:
                    device = 'cpu'
                    Logger.info("🎨 Initializing aesthetic model (MUSIQ-AVA) on CPU (slow)...")
                
                cls._aesthetic_metric = pyiqa.create_metric('musiq-ava', device=device)
                Logger.info("✓ Aesthetic model ready")
            except Exception as e:
                Logger.warning(f"Could not initialize aesthetic model: {e}")
        return cls._aesthetic_metric
    
    @staticmethod
    def compute_aesthetic_quality(image_path: str) -> float:
        """
        Computes aesthetic quality using PyIQA MUSIQ-AVA model
        Returns: 0-1 score (normalized from 1-10 AVA scale)
        Note: Converts HEIC to temp JPG as PyIQA doesn't support HEIC
        """
        try:
            metric = TechnicalImageAnalyzer.get_aesthetic_metric()
            if metric is None:
                return 0.5  # Neutral score if unavailable
            
            # Convert HEIC to temp JPG if needed
            img_path = str(image_path)
            temp_file = None
            
            if img_path.lower().endswith('.heic'):
                from PIL import Image
                from pillow_heif import register_heif_opener
                register_heif_opener()
                import tempfile
                
                img = Image.open(img_path)
                
                # Resize to max 512px for faster processing (maintains aspect ratio)
                max_size = 512
                if max(img.size) > max_size:
                    img.thumbnail((max_size, max_size), Image.Resampling.LANCZOS)
                
                temp_file = tempfile.NamedTemporaryFile(suffix='.jpg', delete=False)
                img.save(temp_file.name, 'JPEG', quality=85, optimize=True)
                img_path = temp_file.name
            
            score = metric(img_path)
            
            # Cleanup temp file
            if temp_file:
                import os
                try:
                    os.unlink(temp_file.name)
                except:
                    pass
            
            # MUSIQ-AVA returns 1-10 (AVA aesthetic scale), normalize to 0-1
            normalized = (float(score.item()) - 1.0) / 9.0  # (score-1)/9 maps 1-10 to 0-1
            return max(0.0, min(1.0, normalized))
            
        except Exception as e:
            Logger.error(f"Error computing aesthetic quality: {e}")
            return 0.5
    
    @staticmethod
    def compute_perceptual_hash(image_path: str, hash_size: int = 8) -> str:
        """
        Calcula perceptual hash (pHash) para detecção de duplicatas
        Retorna: hex string do hash
        """
        try:
            from PIL import Image
            import cv2
            
            # Carrega e converte para escala de cinza
            img = Image.open(image_path).convert('L')
            img = img.resize((hash_size * 4, hash_size * 4), Image.Resampling.LANCZOS)
            
            # Converte para array numpy
            pixels = np.array(img, dtype=np.float32)
            
            # DCT (Discrete Cosine Transform)
            dct = cv2.dct(pixels)
            dct_low = dct[:hash_size, :hash_size]
            
            # Mediana
            med = np.median(dct_low)
            
            # Hash binário
            hash_bits = dct_low > med
            hash_int = sum([2**i for (i, v) in enumerate(hash_bits.flatten()) if v])
            
            return f"{hash_int:016x}"
            
        except Exception as e:
            Logger.error(f"Erro ao calcular pHash: {e}")
            return "0" * 16
    
    @staticmethod
    def compute_sharpness(image_path: str, exif_metadata: Optional[Dict] = None) -> Dict[str, Any]:
        """
        Analyzes sharpness with depth-of-field detection and camera context
        Args:
            image_path: Path to image
            exif_metadata: Optional EXIF data for context
        Returns: {
            'global_sharpness': 0-1,
            'max_local_sharpness': 0-1,
            'sharpness_variance': float,
            'has_intentional_dof': bool,
            'motion_blur_risk': bool,
            'overall_score': 0-1,
            'analysis_notes': List[str]
        }
        """
        try:
            import cv2
            
            # Load grayscale
            img = cv2.imread(str(image_path), cv2.IMREAD_GRAYSCALE)
            if img is None:
                from PIL import Image
                pil_img = Image.open(image_path).convert('L')
                img = np.array(pil_img)
            
            # Global sharpness (Laplacian variance)
            laplacian = cv2.Laplacian(img, cv2.CV_64F)
            global_variance = laplacian.var()
            global_sharpness = min(global_variance / 100.0, 1.0)
            
            # Local sharpness analysis (divide into 9 regions)
            h, w = img.shape
            region_h, region_w = h // 3, w // 3
            
            local_sharpness = []
            for i in range(3):
                for j in range(3):
                    region = img[i*region_h:(i+1)*region_h, j*region_w:(j+1)*region_w]
                    region_lap = cv2.Laplacian(region, cv2.CV_64F)
                    local_var = region_lap.var()
                    local_sharpness.append(local_var)
            
            # Statistics
            max_local = max(local_sharpness)
            max_local_normalized = min(max_local / 100.0, 1.0)
            sharpness_std = np.std(local_sharpness)
            mean_local = np.mean(local_sharpness)
            
            # Analysis notes
            notes = []
            
            # Camera context analysis
            expected_dof = False
            motion_blur_risk = False
            
            if exif_metadata:
                # Wide aperture = shallow DoF expected
                aperture = exif_metadata.get('aperture')
                if aperture and aperture <= 2.8:
                    expected_dof = True
                    notes.append(f"Wide aperture (f/{aperture:.1f}) - shallow DoF expected")
                elif aperture:
                    notes.append(f"Aperture: f/{aperture:.1f}")
                
                # Telephoto lens = shallower DoF
                focal_length = exif_metadata.get('focal_length')
                if focal_length and focal_length > 50:
                    expected_dof = True
                    notes.append(f"Telephoto ({focal_length:.0f}mm) - shallow DoF expected")
                elif focal_length:
                    notes.append(f"Focal length: {focal_length:.0f}mm")
                
                # Slow shutter = motion blur risk
                shutter = exif_metadata.get('shutter_speed')
                if shutter:
                    # Rule of thumb: 1/focal_length for handheld
                    if focal_length:
                        safe_shutter = 1.0 / focal_length
                        if shutter > safe_shutter * 2:  # 2x slower than safe
                            motion_blur_risk = True
                            notes.append(f"Slow shutter ({shutter:.3f}s) - motion blur risk")
                    elif shutter > 1/30:  # General handheld threshold
                        motion_blur_risk = True
                        notes.append(f"Slow shutter ({shutter:.3f}s) - motion blur risk")
                    else:
                        notes.append(f"Shutter: 1/{int(1/shutter)}s")
                
                # High ISO = potential noise/shake
                iso = exif_metadata.get('iso')
                if iso and iso > 1600:
                    notes.append(f"High ISO ({iso}) - noise possible")
                elif iso:
                    notes.append(f"ISO: {iso}")
            
            # Intentional DoF detection (enhanced with camera context):
            # 1. High variance in local sharpness (some regions sharp, others blurred)
            # 2. At least one region is very sharp (>80)
            # 3. Significant difference between max and mean (>30)
            # 4. OR camera settings indicate expected shallow DoF
            visual_dof = (
                sharpness_std > 30 and 
                max_local > 80 and 
                (max_local - mean_local) > 30
            )
            
            has_intentional_dof = visual_dof or expected_dof
            
            if visual_dof and not expected_dof:
                notes.append("Visual DoF detected (subject sharp, background blurred)")
            elif expected_dof and not visual_dof:
                notes.append("DoF expected from camera settings but not visually prominent")
            elif visual_dof and expected_dof:
                notes.append("Confirmed intentional DoF (visual + camera settings)")
            
            # Overall score:
            # If intentional DoF, use max local sharpness (subject is sharp)
            # If motion blur risk and low global sharpness, penalize
            # Otherwise, use global sharpness
            if has_intentional_dof:
                overall_score = max_local_normalized
            elif motion_blur_risk and global_sharpness < 0.5:
                overall_score = global_sharpness * 0.7  # Penalize motion blur
                notes.append("Quality reduced due to motion blur")
            else:
                overall_score = global_sharpness
            
            return {
                'global_sharpness': float(global_sharpness),
                'max_local_sharpness': float(max_local_normalized),
                'sharpness_variance': float(sharpness_std),
                'has_intentional_dof': bool(has_intentional_dof),
                'motion_blur_risk': bool(motion_blur_risk),
                'overall_score': float(overall_score),
                'analysis_notes': notes
            }
            
        except Exception as e:
            Logger.error(f"Error computing sharpness: {e}")
            return {
                'global_sharpness': 0.0,
                'max_local_sharpness': 0.0,
                'sharpness_variance': 0.0,
                'has_intentional_dof': False,
                'motion_blur_risk': False,
                'overall_score': 0.0,
                'analysis_notes': []
            }
    
    @staticmethod
    def analyze_exposure(image_path: str) -> Dict[str, Any]:
        """
        Analisa qualidade de exposição
        Retorna: {quality_score, highlights_clipped, shadows_blocked}
        """
        try:
            import cv2
            from PIL import Image
            
            # Carrega imagem
            try:
                img = cv2.imread(str(image_path))
                if img is None:
                    pil_img = Image.open(image_path).convert('RGB')
                    img = cv2.cvtColor(np.array(pil_img), cv2.COLOR_RGB2BGR)
            except:
                pil_img = Image.open(image_path).convert('RGB')
                img = cv2.cvtColor(np.array(pil_img), cv2.COLOR_RGB2BGR)
            
            # Converte para escala de cinza
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            
            # Histograma
            hist = cv2.calcHist([gray], [0], None, [256], [0, 256])
            total_pixels = gray.shape[0] * gray.shape[1]
            
            # Clipping (>2% dos pixels em extremos)
            highlights_clipped = (hist[250:].sum() / total_pixels) > 0.02
            shadows_blocked = (hist[:5].sum() / total_pixels) > 0.02
            
            # Quality score baseado em distribuição
            # Boa exposição = histograma bem distribuído
            hist_normalized = hist / total_pixels
            entropy = -np.sum(hist_normalized * np.log2(hist_normalized + 1e-10))
            quality_score = min(entropy / 8.0, 1.0)  # Max entropy ~8 bits
            
            return {
                "quality_score": float(quality_score),
                "highlights_clipped": bool(highlights_clipped),
                "shadows_blocked": bool(shadows_blocked)
            }
            
        except Exception as e:
            Logger.error(f"Erro ao analisar exposição: {e}")
            return {"quality_score": 0.5, "highlights_clipped": False, "shadows_blocked": False}


# ============================================================================
# PHOTO CONTEXT ANALYZER V2
# ============================================================================

class PhotoContextAnalyzer:
    """Analisador contextual de fotos com SKP"""
    
    def __init__(self, cache_dir: str = ".photoguru_cache"):
        self.cache_dir = Path(cache_dir)
        self.cache_dir.mkdir(exist_ok=True)
        
        self.photos: List[PhotoMetadata] = []
        self.groups: List[PhotoGroup] = []
        self.contextual_conclusion: Optional[ContextualConclusion] = None
        
        # CLIP analyzer
        if CLIP_AVAILABLE:
            Logger.info("🔧 Inicializando CLIP...")
            self.clip_analyzer = CLIPAnalyzer()
        else:
            self.clip_analyzer = None
    
    def add_photo(self, photo: PhotoMetadata) -> None:
        """Adiciona foto ao analisador"""
        self.photos.append(photo)
    
    def create_groups(self) -> List[PhotoGroup]:
        """
        Cria grupos baseados em:
        1. Temporal: <= 2 horas (se disponível)
        2. Espacial: < 1km
        3. Visual: similaridade CLIP
        """
        if not self.photos:
            return []
        
        Logger.info("📦 Criando grupos de fotos...")
        
        # Separa fotos com e sem data
        photos_with_date = [p for p in self.photos if p.datetime_original]
        photos_without_date = [p for p in self.photos if not p.datetime_original]
        
        Logger.info(f"   {len(photos_with_date)} fotos com data, {len(photos_without_date)} sem data")
        
        # Se não há datas, agrupa apenas por localização e similaridade visual
        if not photos_with_date:
            Logger.warning("   ⚠️ Nenhuma foto tem metadados de data! Agrupando por localização e similaridade visual...")
            return self._group_by_location_and_visual(self.photos)
        
        # Ordena fotos com data
        sorted_photos = sorted(photos_with_date, key=lambda x: x.datetime_original)
        
        groups = []
        current_group = []
        group_start_time = None
        group_location = None
        
        for photo in sorted_photos:
            # Primeira foto do grupo
            if not current_group:
                current_group.append(photo)
                group_start_time = photo.datetime_original
                group_location = (photo.gps_lat, photo.gps_lon) if photo.gps_lat else None
                continue
            
            # Verifica critérios de agrupamento
            time_diff = (photo.datetime_original - group_start_time).total_seconds() / 3600
            
            location_match = True
            if group_location and photo.gps_lat and photo.gps_lon:
                dist = self._haversine_distance(
                    group_location[0], group_location[1],
                    photo.gps_lat, photo.gps_lon
                )
                location_match = dist < 1.0
            
            # Critério: MESMO tempo (<=2h) E MESMA localização (<1km)
            # OU tempo próximo mas localização diferente (novo lugar = novo grupo)
            if time_diff <= 2.0 and location_match:
                current_group.append(photo)
            else:
                # Finaliza grupo atual
                if current_group:
                    groups.append(self._create_group(current_group))
                
                current_group = [photo]
                group_start_time = photo.datetime_original
                group_location = (photo.gps_lat, photo.gps_lon) if photo.gps_lat else None
        
        # Adiciona último grupo
        if current_group:
            groups.append(self._create_group(current_group))
        
        # Adiciona fotos sem data ao grupo mais próximo espacialmente
        if photos_without_date:
            Logger.info(f"   Adicionando {len(photos_without_date)} fotos sem data aos grupos...")
            for photo in photos_without_date:
                if photo.gps_lat and photo.gps_lon:
                    # Encontra grupo mais próximo
                    best_group = None
                    min_dist = float('inf')
                    
                    for group in groups:
                        if group.location_coords:
                            dist = self._haversine_distance(
                                photo.gps_lat, photo.gps_lon,
                                group.location_coords[0], group.location_coords[1]
                            )
                            if dist < min_dist:
                                min_dist = dist
                                best_group = group
                    
                    if best_group and min_dist < 1.0:  # < 1km
                        best_group.photos.append(photo)
                        photo.group_id = best_group.group_id
        
        self.groups = groups
        Logger.success(f"✅ {len(groups)} grupos criados")
        return groups
    
    def _group_by_location_and_visual(self, photos: List[PhotoMetadata]) -> List[PhotoGroup]:
        """
        Agrupa fotos por localização e similaridade visual (quando não há data)
        """
        from sklearn.cluster import DBSCAN
        
        # Usa CLIP embeddings para clustering
        embeddings = []
        valid_photos = []
        
        for photo in photos:
            if photo.clip_embedding is not None:
                embeddings.append(photo.clip_embedding)
                valid_photos.append(photo)
        
        if not embeddings:
            # Fallback: um grupo único
            group = PhotoGroup(
                group_id="group_0",
                photos=photos,
                start_time=None,
                end_time=None
            )
            for photo in photos:
                photo.group_id = "group_0"
            return [group]
        
        # Clustering por similaridade visual
        embeddings_matrix = np.vstack(embeddings)
        clustering = DBSCAN(eps=0.3, min_samples=2, metric='cosine').fit(embeddings_matrix)
        
        # Cria grupos
        groups_dict = defaultdict(list)
        for idx, label in enumerate(clustering.labels_):
            if label == -1:
                # Noise: cria grupo individual
                group_id = f"group_single_{idx}"
            else:
                group_id = f"group_{label}"
            
            groups_dict[group_id].append(valid_photos[idx])
        
        # Converte para PhotoGroup
        groups = []
        for group_id, group_photos in groups_dict.items():
            group = PhotoGroup(
                group_id=group_id,
                photos=group_photos,
                start_time=None,
                end_time=None
            )
            for photo in group_photos:
                photo.group_id = group_id
            groups.append(group)
        
        return groups
    
    def _create_group(self, photos: List[PhotoMetadata]) -> PhotoGroup:
        """Cria PhotoGroup a partir de lista de fotos"""
        times = [p.datetime_original for p in photos if p.datetime_original]
        locations = [(p.gps_lat, p.gps_lon) for p in photos if p.gps_lat and p.gps_lon]
        
        group_id = self._generate_group_id(photos[0].datetime_original, photos[0].filepath)
        
        # Atribui group_id às fotos
        for photo in photos:
            photo.group_id = group_id
        
        return PhotoGroup(
            group_id=group_id,
            photos=photos,
            start_time=min(times) if times else None,
            end_time=max(times) if times else None,
            location_coords=locations[0] if locations else None
        )
    
    def analyze_groups_with_llm(self, llm_client: Any) -> None:
        """
        Analisa cada grupo com LLM VENDO IMAGENS-CHAVE para entender narrativa
        """
        Logger.info("🧠 Analisando grupos com LLM...")
        
        for i, group in enumerate(self.groups, 1):
            Logger.info(f"   📋 Grupo {i}/{len(self.groups)} ({len(group.photos)} fotos)")
            
            # Ordena fotos por tempo
            sorted_photos = sorted(group.photos, key=lambda p: p.datetime_original or datetime.min)
            
            # Constrói contexto rico de CADA foto (já analisadas no Pass 2)
            photos_context = []
            for idx, photo in enumerate(sorted_photos, 1):
                time_str = photo.datetime_original.strftime('%H:%M:%S') if photo.datetime_original else 'Horário desconhecido'
                loc_str = photo.location_name if photo.location_name else 'Localização desconhecida'
                
                # Pega título e descrição JÁ GERADOS pelo LLM no Pass 2
                title = photo.llm_title if photo.llm_title else 'Sem título'
                description = photo.llm_description if photo.llm_description else 'Sem descrição'
                
                photos_context.append(
                    f"[{idx}/{len(sorted_photos)}] {photo.filename}\n"
                    f"  Horário: {time_str}\n"
                    f"  Local: {loc_str}\n"
                    f"  Título: {title}\n"
                    f"  Descrição: {description}"
                )
            
            # Prepara contexto textual completo
            context_text = "\n\n".join(photos_context)
            duration = group.duration_minutes()
            
            # Prompt focado em NARRATIVA TEMPORAL usando o contexto rico
            prompt = f"""Analise esta sequência de {len(sorted_photos)} fotos tiradas ao longo de {duration:.0f} minutos.

CONTEXTO - Fotos em ordem cronológica com suas descrições detalhadas:
{context_text}

TAREFA: Entender a NARRATIVA e JORNADA a partir do contexto
- Olhe a progressão TEMPORAL (o que acontece primeiro? o que acontece depois?)
- Olhe as mudanças de LOCALIZAÇÃO (saindo de onde para onde?)
- Olhe as mudanças de ATIVIDADE (que transições existem na sequência?)

EXEMPLO: Se foto 1 mostra "interior do carro", fotos 2-4 mostram "dirigindo pelas ruas", e fotos 5-6 mostram "praça da igreja" → 
isso é uma JORNADA para visitar a igreja, NÃO apenas "momentos de viagem"

Forneça análise CONTEXTUAL em JSON:
{{
  "summary": "Descreva a NARRATIVA/JORNADA específica com progressão temporal (seja específico sobre o que acontece)",
  "event_type": "Nome ESPECÍFICO do evento (ex: 'Visita à Igreja do Bonfim', não apenas 'Viagem')",
  "common_themes": ["temas", "específicos", "contextuais"],
  "main_location": "Localização principal",
  "social_context": "Contexto social observado"
}}"""
            
            # Envia para LLM apenas com contexto textual rico
            response = llm_client.chat(
                user_message=prompt,
                system_context="Você analisa sequências de fotos entendendo narrativas temporais e jornadas a partir do contexto fornecido."
            )
            
            if response and 'content' in response:
                try:
                    analysis = self._extract_json(response['content'])
                    group.llm_summary = analysis.get('summary')
                    group.llm_event_type = analysis.get('event_type')
                    group.llm_common_themes = analysis.get('common_themes', [])
                    Logger.info(f"      ✓ {group.llm_event_type}: {group.llm_summary[:60]}...")
                except:
                    Logger.warning(f"      ⚠️ Falha ao parsear resposta")
        
        Logger.success("✅ Análise de grupos concluída")
    
    def create_global_conclusion(self, llm_client: Any) -> ContextualConclusion:
        """
        Cria conclusão contextual global analisando todos os grupos
        """
        Logger.info("🌍 Criando conclusão contextual global...")
        
        # Prepara resumo de todos os grupos
        groups_summary = []
        for group in self.groups:
            groups_summary.append({
                "group_id": group.group_id,
                "photos_count": len(group.photos),
                "time_range": f"{group.start_time} to {group.end_time}" if group.start_time else "unknown",
                "duration_minutes": group.duration_minutes(),
                "event_type": group.llm_event_type,
                "summary": group.llm_summary,
                "themes": group.llm_common_themes
            })
        
        prompt = f"""Analise este acervo completo de fotos organizado em {len(self.groups)} grupos:

{json.dumps(groups_summary, indent=2, default=str)}

Forneça uma conclusão contextual geral em JSON:
{{
    "overall_summary": "Resumo geral do acervo (2-3 frases)",
    "main_themes": ["tema1", "tema2", "tema3"],
    "time_periods": ["período1", "período2"],
    "main_locations": ["local1", "local2", "local3"],
    "social_patterns": ["padrão1", "padrão2"]
}}"""
        
        response = llm_client.chat(prompt, system_context="Você é um especialista em análise de memórias fotográficas.")
        
        if response and 'content' in response:
            content = response['content']
            try:
                analysis = self._extract_json(content)
                
                # Cria conclusão contextual
                conclusion = ContextualConclusion(
                    total_photos=len(self.photos),
                    groups_count=len(self.groups),
                    llm_overall_summary=analysis.get('overall_summary', ''),
                    llm_main_themes=analysis.get('main_themes', []),
                    llm_time_periods=analysis.get('time_periods', []),
                    llm_main_locations=analysis.get('main_locations', []),
                    llm_social_patterns=analysis.get('social_patterns', []),
                    skp_global_key=None  # Será criado depois
                )
                
                self.contextual_conclusion = conclusion
                Logger.success(f"✅ Conclusão global: {conclusion.llm_overall_summary[:80]}...")
                return conclusion
                
            except Exception as e:
                Logger.error(f"Erro ao criar conclusão global: {e}")
                raise
    
    def create_skp_keys(self) -> None:
        """
        Cria Semantic Keys (SKP) para:
        - Cada grupo
        - Global (acervo inteiro)
        """
        Logger.info("🔑 Criando Semantic Keys (SKP)...")
        
        # 1. Chave global do acervo
        if self.contextual_conclusion and self.photos:
            # Agrega embeddings de todas as fotos
            all_embeddings = [p.clip_embedding for p in self.photos if p.clip_embedding is not None]
            
            if all_embeddings:
                global_vector = np.mean(all_embeddings, axis=0)
                global_key = SemanticKey(
                    vector=global_vector,
                    key_id="global",
                    role="anchor",
                    metadata={
                        "type": "global_collection",
                        "total_photos": len(self.photos),
                        "summary": self.contextual_conclusion.llm_overall_summary
                    }
                )
                
                self.contextual_conclusion.skp_global_key = global_key
                Logger.info(f"   🌍 Chave global criada")
        
        # 2. Chave para cada grupo
        for group in self.groups:
            group_embeddings = [p.clip_embedding for p in group.photos if p.clip_embedding is not None]
            
            if group_embeddings:
                group_vector = np.mean(group_embeddings, axis=0)
                group_key = SemanticKey(
                    vector=group_vector,
                    key_id=group.group_id,
                    role="anchor",
                    metadata={
                        "type": "group",
                        "event_type": group.llm_event_type,
                        "photos_count": len(group.photos),
                        "summary": group.llm_summary
                    }
                )
                
                group.skp_group_key = group_key
                
                # Atribui referência às fotos do grupo
                for photo in group.photos:
                    if photo.skp_group_keys is None:
                        photo.skp_group_keys = []
                    photo.skp_group_keys.append(group.group_id)
        
        Logger.success("✅ Semantic Keys criadas")
    
    def _prepare_group_context(self, group: PhotoGroup) -> str:
        """Prepara contexto textual de um grupo para o LLM"""
        lines = []
        lines.append(f"Grupo: {group.group_id}")
        lines.append(f"Total de fotos: {len(group.photos)}")
        
        if group.start_time:
            lines.append(f"Período: {group.start_time} a {group.end_time}")
            lines.append(f"Duração: {group.duration_minutes():.0f} minutos")
        
        if group.location_coords:
            lines.append(f"Localização: {group.location_coords}")
        
        # Features CLIP detectadas
        all_features = []
        for photo in group.photos:
            if photo.clip_features:
                all_features.extend(photo.clip_features)
        
        if all_features:
            feature_counts = defaultdict(int)
            for f in all_features:
                feature_counts[f] += 1
            
            top_features = sorted(feature_counts.items(), key=lambda x: x[1], reverse=True)[:10]
            lines.append(f"Features detectadas: {', '.join([f'{k}({v})' for k, v in top_features])}")
        
        # Faces
        total_faces = sum([p.face_count or 0 for p in group.photos])
        if total_faces > 0:
            lines.append(f"Total de faces detectadas: {total_faces}")
        
        return "\n".join(lines)
    
    def _extract_json(self, text: str) -> Dict[str, Any]:
        """Extrai JSON de resposta do LLM"""
        # Tenta encontrar bloco JSON
        match = re.search(r'\{.*\}', text, re.DOTALL)
        if match:
            return json.loads(match.group(0))
        else:
            return json.loads(text)
    
    @staticmethod
    def _haversine_distance(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
        """Calcula distância entre coordenadas GPS em km"""
        from math import radians, sin, cos, sqrt, atan2
        
        R = 6371
        lat1, lon1, lat2, lon2 = map(radians, [lat1, lon1, lat2, lon2])
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        
        a = sin(dlat/2)**2 + cos(lat1) * cos(lat2) * sin(dlon/2)**2
        c = 2 * atan2(sqrt(a), sqrt(1-a))
        
        return R * c
    
    @staticmethod
    def _generate_group_id(dt: Optional[datetime], filepath: str) -> str:
        """Gera ID único para grupo"""
        if dt:
            base = f"{dt.strftime('%Y%m%d_%H%M')}_{Path(filepath).stem}"
        else:
            base = f"unknown_{Path(filepath).stem}"
        
        return hashlib.md5(base.encode()).hexdigest()[:12]


# ============================================================================
# LOGGER
# ============================================================================

class Logger:
    """Sistema de logging"""
    
    @staticmethod
    def info(message: str):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] INFO: {message}")
    
    @staticmethod
    def error(message: str):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] ERROR: {message}", file=sys.stderr)
    
    @staticmethod
    def success(message: str):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] SUCCESS: {message}")
    
    @staticmethod
    def warning(message: str):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] WARNING: {message}")


# ============================================================================
# TERMINAL TOOLS
# ============================================================================

class TerminalTools:
    """Ferramentas para operações no terminal"""
    
    @staticmethod
    def read_image_metadata(filepath: str) -> Dict[str, Any]:
        """Lê metadados EXIF de uma imagem usando exifread (suporta HEIC)"""
        try:
            import exifread
            
            with open(filepath, 'rb') as f:
                tags = exifread.process_file(f, details=False)
            
            metadata = {}
            
            # Data/hora
            datetime_str = tags.get('EXIF DateTimeOriginal') or tags.get('Image DateTime')
            if datetime_str:
                try:
                    dt = datetime.strptime(str(datetime_str), "%Y:%m:%d %H:%M:%S")
                    metadata['datetime_original'] = dt
                except:
                    pass
            
            # Câmera
            camera_make = tags.get('Image Make')
            if camera_make:
                metadata['camera_make'] = str(camera_make)
            
            camera_model = tags.get('Image Model')
            if camera_model:
                metadata['camera_model'] = str(camera_model)
            
            # GPS
            coords = TerminalTools._extract_gps_coordinates(tags)
            if coords:
                metadata['gps_lat'] = coords['latitude']
                metadata['gps_lon'] = coords['longitude']
            
            # Camera settings (for quality analysis context)
            # Aperture
            aperture = tags.get('EXIF FNumber')
            if aperture:
                try:
                    # FNumber is usually a ratio like "14/10" = f/1.4
                    if '/' in str(aperture):
                        parts = str(aperture).split('/')
                        metadata['aperture'] = float(parts[0]) / float(parts[1])
                    else:
                        metadata['aperture'] = float(aperture)
                except:
                    pass
            
            # Focal length
            focal = tags.get('EXIF FocalLength')
            if focal:
                try:
                    if '/' in str(focal):
                        parts = str(focal).split('/')
                        metadata['focal_length'] = float(parts[0]) / float(parts[1])
                    else:
                        metadata['focal_length'] = float(focal)
                except:
                    pass
            
            # ISO
            iso = tags.get('EXIF ISOSpeedRatings')
            if iso:
                try:
                    metadata['iso'] = int(str(iso))
                except:
                    pass
            
            # Shutter speed
            shutter = tags.get('EXIF ExposureTime')
            if shutter:
                try:
                    if '/' in str(shutter):
                        parts = str(shutter).split('/')
                        metadata['shutter_speed'] = float(parts[0]) / float(parts[1])
                    else:
                        metadata['shutter_speed'] = float(shutter)
                except:
                    pass
            
            return metadata
            
        except Exception as e:
            Logger.error(f"Erro ao ler metadados: {e}")
            return {}
    
    @staticmethod
    def _convert_gps_to_degrees(value):
        """Converte coordenadas GPS para graus decimais"""
        d, m, s = value
        return float(d) + float(m) / 60.0 + float(s) / 3600.0
    
    @staticmethod
    def _extract_gps_coordinates(tags: Dict) -> Optional[Dict[str, float]]:
        """Extrai coordenadas GPS de tags EXIF"""
        try:
            lat_data = tags.get('GPS GPSLatitude')
            lat_ref = tags.get('GPS GPSLatitudeRef')
            lon_data = tags.get('GPS GPSLongitude')
            lon_ref = tags.get('GPS GPSLongitudeRef')
            
            if not all([lat_data, lat_ref, lon_data, lon_ref]):
                return None
            
            def convert_to_degrees(value):
                """Converte EXIF GPS para decimal"""
                parts = str(value).strip('[]').split(',')
                if len(parts) >= 3:
                    d = float(parts[0].split('/')[0]) / float(parts[0].split('/')[1]) if '/' in parts[0] else float(parts[0])
                    m = float(parts[1].split('/')[0]) / float(parts[1].split('/')[1]) if '/' in parts[1] else float(parts[1])
                    s = float(parts[2].split('/')[0]) / float(parts[2].split('/')[1]) if '/' in parts[2] else float(parts[2])
                    return d + m / 60.0 + s / 3600.0
                return 0.0
            
            latitude = convert_to_degrees(lat_data)
            longitude = convert_to_degrees(lon_data)
            
            if str(lat_ref) == 'S':
                latitude = -latitude
            if str(lon_ref) == 'W':
                longitude = -longitude
            
            return {'latitude': latitude, 'longitude': longitude}
            
        except Exception as e:
            return None
    
    @staticmethod
    def get_location_from_gps(lat: float, lon: float) -> Optional[str]:
        """Geocoding reverso usando Nominatim"""
        try:
            url = "https://nominatim.openstreetmap.org/reverse"
            params = {
                'lat': lat,
                'lon': lon,
                'format': 'json',
                'zoom': 18,
                'addressdetails': 1
            }
            headers = {'User-Agent': 'PhotoGuru-Agent-v2/1.0'}
            
            response = requests.get(url, params=params, headers=headers, timeout=5)
            
            if response.status_code == 200:
                data = response.json()
                
                if 'address' in data:
                    addr = data['address']
                    
                    # Constrói localização hierárquica
                    parts = []
                    
                    # POI ou local específico
                    if 'amenity' in addr:
                        parts.append(addr['amenity'])
                    elif 'tourism' in addr:
                        parts.append(addr['tourism'])
                    
                    # Bairro/Cidade
                    if 'suburb' in addr:
                        parts.append(addr['suburb'])
                    elif 'neighbourhood' in addr:
                        parts.append(addr['neighbourhood'])
                    
                    # Cidade
                    if 'city' in addr:
                        parts.append(addr['city'])
                    elif 'town' in addr:
                        parts.append(addr['town'])
                    elif 'village' in addr:
                        parts.append(addr['village'])
                    
                    # Estado
                    if 'state' in addr:
                        parts.append(addr['state'])
                    
                    # País
                    if 'country' in addr:
                        parts.append(addr['country'])
                    
                    return ", ".join(parts) if parts else data.get('display_name')
            
            return None
            
        except Exception as e:
            Logger.warning(f"Erro no geocoding: {e}")
            return None
    
    @staticmethod
    def detect_faces(filepath: str) -> Dict[str, Any]:
        """Detecta faces em uma imagem com filtros de qualidade"""
        if not FACE_RECOGNITION_AVAILABLE:
            return {"face_count": 0, "face_locations": [], "embeddings": []}
        
        try:
            from PIL import Image
            
            img_pil = Image.open(filepath)
            
            if img_pil.mode != 'RGB':
                img_pil = img_pil.convert('RGB')
            
            img_array = np.array(img_pil)
            img_cv = cv2.cvtColor(img_array, cv2.COLOR_RGB2BGR)
            gray = cv2.cvtColor(img_cv, cv2.COLOR_BGR2GRAY)
            img_height, img_width = gray.shape
            
            cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
            face_cascade = cv2.CascadeClassifier(cascade_path)
            
            # Detecta faces com parâmetros mais rigorosos
            faces = face_cascade.detectMultiScale(
                gray, 
                scaleFactor=1.1, 
                minNeighbors=6,  # Moderado (entre 5 e 8)
                minSize=(50, 50)  # Moderado (entre 30x30 e 60x60)
            )
            
            # Filtra faces por qualidade
            quality_faces = []
            embeddings = []
            
            for (x, y, w, h) in faces:
                # Filtro 1: Tamanho mínimo relativo (pelo menos 3% da largura da imagem)
                if w < img_width * 0.03 or h < img_height * 0.03:
                    continue
                
                # Filtro 2: Proporção razoável (faces não devem ser muito alongadas)
                aspect_ratio = w / h
                if aspect_ratio < 0.5 or aspect_ratio > 1.5:
                    continue
                
                # Filtro 3: Nitidez (variância de Laplacian - faces desfocadas têm baixa variância)
                face_roi = gray[y:y+h, x:x+w]
                laplacian_var = cv2.Laplacian(face_roi, cv2.CV_64F).var()
                if laplacian_var < 30:  # Threshold reduzido (era 50)
                    continue
                
                # Face passou nos filtros - computa embedding
                face_resized = cv2.resize(face_roi, (64, 64))
                hist = cv2.calcHist([face_resized], [0], None, [256], [0, 256])
                hist = hist.flatten()
                hist = hist / (np.linalg.norm(hist) + 1e-7)
                
                quality_faces.append((int(x), int(y), int(w), int(h)))
                embeddings.append(hist)
            
            return {
                "face_count": len(quality_faces),
                "face_locations": quality_faces,
                "embeddings": embeddings
            }
            
        except Exception as e:
            Logger.warning(f"Face detection failed: {e}")
            return {"face_count": 0, "face_locations": [], "embeddings": []}
    
    @staticmethod
    def recognize_faces_in_batch(photos: List[PhotoMetadata]) -> Dict[str, Any]:
        """Reconhece e agrupa faces iguais em múltiplas fotos"""
        if not FACE_RECOGNITION_AVAILABLE:
            return {"person_clusters": {}, "photo_to_persons": {}}
        
        try:
            from sklearn.cluster import DBSCAN
            
            all_embeddings = []
            embedding_to_photo = []  # (photo_idx, face_idx)
            
            for photo_idx, photo in enumerate(photos):
                faces = TerminalTools.detect_faces(photo.filepath)
                if faces['embeddings']:
                    for face_idx, emb in enumerate(faces['embeddings']):
                        all_embeddings.append(emb)
                        embedding_to_photo.append((photo_idx, face_idx))
            
            if len(all_embeddings) < 2:
                return {"person_clusters": {}, "photo_to_persons": {}}
            
            # Clustering - normalize and validate embeddings
            embeddings_matrix = np.vstack(all_embeddings)
            
            # Remove qualquer inf/nan e normaliza
            embeddings_matrix = np.nan_to_num(embeddings_matrix, nan=0.0, posinf=0.0, neginf=0.0)
            norms = np.linalg.norm(embeddings_matrix, axis=1, keepdims=True)
            norms = np.maximum(norms, 1e-10)  # Evita divisão por zero
            embeddings_matrix = embeddings_matrix / norms
            
            # Valida novamente após normalização
            embeddings_matrix = np.nan_to_num(embeddings_matrix, nan=0.0, posinf=0.0, neginf=0.0)
            
            # Usa distância euclidiana (mais estável) já que os vetores estão normalizados
            # eps maior porque distância euclidiana em vetores unitários varia de 0 a 2
            # min_samples=3 significa que precisa de pelo menos 3 faces similares para formar cluster
            clustering = DBSCAN(eps=0.6, min_samples=3, metric='euclidean').fit(embeddings_matrix)
            
            # Agrupa por cluster (pessoa) - usa set para evitar duplicatas de fotos
            person_clusters = defaultdict(set)
            photo_to_persons = defaultdict(set)
            
            for idx, label in enumerate(clustering.labels_):
                if label == -1:  # Noise (face única/não recorrente)
                    continue  # Ignora faces que aparecem pouco
                
                person_id = f"person_{label}"
                photo_idx, face_idx = embedding_to_photo[idx]
                person_clusters[person_id].add(photos[photo_idx].filename)
                photo_to_persons[photos[photo_idx].filename].add(person_id)
            
            # Filtra: só mantém pessoas que aparecem em pelo menos 2 fotos diferentes
            filtered_clusters = {}
            for person_id, photo_set in person_clusters.items():
                if len(photo_set) >= 2:
                    filtered_clusters[person_id] = list(photo_set)
            
            # Atualiza photo_to_persons apenas com pessoas válidas
            filtered_photo_to_persons = defaultdict(set)
            for person_id, photo_list in filtered_clusters.items():
                for photo_filename in photo_list:
                    filtered_photo_to_persons[photo_filename].add(person_id)
            
            filtered_photo_to_persons = {k: list(v) for k, v in filtered_photo_to_persons.items()}
            
            return {
                "person_clusters": filtered_clusters,
                "photo_to_persons": filtered_photo_to_persons
            }
            
        except Exception as e:
            Logger.warning(f"Face recognition failed: {e}")
            return {"person_clusters": {}, "photo_to_persons": {}}
    
    @staticmethod
    def write_image_metadata(photo: PhotoMetadata, output_path: Optional[str] = None) -> bool:
        """
        Escreve metadados do PhotoMetadata de volta para o arquivo da foto usando exiftool
        
        Args:
            photo: PhotoMetadata com todos os dados
            output_path: Path alternativo para salvar (default: sobrescreve original)
        
        Returns:
            True se sucesso, False se erro
        """
        try:
            filepath = output_path or photo.filepath
            file_path = Path(filepath)
            
            if not file_path.exists():
                Logger.error(f"File not found: {filepath}")
                return False
            
            # Verifica exiftool
            result = subprocess.run(['which', 'exiftool'], capture_output=True)
            if result.returncode != 0:
                Logger.error("exiftool not found. Install with: brew install exiftool")
                return False
            
            cmd = ['exiftool', '-overwrite_original']
            updates = []
            
            # Marca como gerado por PhotoGuru
            cmd.extend([
                '-XMP-xmp:CreatorTool=PhotoGuru AI Agent v2',
                '-XMP-photoshop:Credit=Generated by PhotoGuru AI'
            ])
            
            # Título (do LLM)
            if photo.llm_title:
                cmd.extend([
                    f'-XMP:Title={photo.llm_title}',
                    f'-IPTC:ObjectName={photo.llm_title}',
                    f'-XMP-dc:Title={photo.llm_title}'
                ])
                updates.append(f"Title: {photo.llm_title}")
            
            # Descrição (do LLM)
            if photo.llm_description:
                cmd.extend([
                    f'-XMP:Description={photo.llm_description}',
                    f'-EXIF:ImageDescription={photo.llm_description}',
                    f'-IPTC:Caption-Abstract={photo.llm_description}',
                    f'-XMP-dc:Description={photo.llm_description}'
                ])
                updates.append(f"Description: {photo.llm_description[:50]}...")
            
            # Keywords (do LLM)
            if photo.llm_keywords:
                for i, kw in enumerate(photo.llm_keywords):
                    kw_escaped = kw.replace('"', '\\"')
                    if i == 0:
                        cmd.append(f'-XMP-dc:Subject={kw_escaped}')
                        cmd.append(f'-IPTC:Keywords={kw_escaped}')
                    else:
                        cmd.append(f'-XMP-dc:Subject+={kw_escaped}')
                        cmd.append(f'-IPTC:Keywords+={kw_escaped}')
                updates.append(f"Keywords: {len(photo.llm_keywords)} tags")
            
            # Category (do LLM)
            if photo.llm_category:
                cmd.extend([
                    f'-IPTC:Category={photo.llm_category}',
                    f'-XMP-photoshop:Category={photo.llm_category}'
                ])
                updates.append(f"Category: {photo.llm_category}")
            
            # Scene (do LLM)
            if photo.llm_scene:
                cmd.append(f'-XMP-iptcExt:LocationShown={photo.llm_scene}')
                cmd.append(f'-IPTC:ContentLocationName={photo.llm_scene}')
                updates.append(f"Scene: {photo.llm_scene}")
            
            # Mood (do LLM)
            if photo.llm_mood:
                for m in photo.llm_mood.split(','):
                    m = m.strip()
                    if m:
                        cmd.append(f'-IPTC:SupplementalCategories+={m}')
                updates.append(f"Mood: {photo.llm_mood}")
            
            # Rating - usa aesthetic score convertido para 1-5
            if photo.aesthetic_score > 0:
                rating = int(photo.aesthetic_score * 5) + 1  # 0-1 → 1-5
                rating = max(1, min(5, rating))
                cmd.extend([
                    f'-XMP:Rating={rating}',
                    f'-Rating={rating}'
                ])
                updates.append(f"Rating: {rating}/5 (aesthetic: {photo.aesthetic_score:.3f})")
            
            # Location
            if photo.location_name:
                parts = photo.location_name.split(', ')
                if len(parts) >= 3:
                    city = parts[0] if len(parts) > 2 else None
                    state = parts[-2] if len(parts) > 1 else None
                    country = parts[-1]
                    
                    if city:
                        cmd.extend([f'-XMP-photoshop:City={city}', f'-IPTC:City={city}'])
                        updates.append(f"City: {city}")
                    if state:
                        cmd.extend([f'-XMP-photoshop:State={state}', f'-IPTC:Province-State={state}'])
                        updates.append(f"State: {state}")
                    if country:
                        cmd.extend([f'-XMP-photoshop:Country={country}', f'-IPTC:Country-PrimaryLocationName={country}'])
                        updates.append(f"Country: {country}")
            
            # Technical metadata em UserComment (JSON compacto)
            tech_data = {
                'sharp': round(photo.tech_sharpness_score, 3),
                'expo': round(photo.tech_exposure_quality, 3),
                'aesth': round(photo.aesthetic_score, 3),
                'qual': round(photo.tech_overall_quality, 3),
                'dup': photo.tech_duplicate_group,
                'burst': photo.tech_burst_group,
                'burst_pos': photo.tech_burst_position,
                'burst_best': photo.tech_is_best_in_burst,
                'faces': photo.face_count
            }
            tech_json = json.dumps(tech_data, separators=(',', ':'))
            cmd.append(f'-EXIF:UserComment=PhotoGuru:{tech_json}')
            updates.append(f"Technical: {len(tech_data)} metrics")
            
            if not updates:
                Logger.warning("No metadata to update")
                return False
            
            cmd.append(str(file_path))
            
            Logger.info(f"Writing metadata: {', '.join(updates[:3])}...")
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                Logger.success(f"✅ Metadata written to: {file_path.name}")
                return True
            else:
                Logger.error(f"exiftool error: {result.stderr}")
                return False
                
        except Exception as e:
            Logger.error(f"Error writing metadata: {e}")
            return False


# ============================================================================
# LM STUDIO CLIENT
# ============================================================================

class LMStudioClient:
    """Cliente para comunicação com LM Studio"""
    
    def __init__(self, base_url: str = "http://localhost:1234/v1"):
        self.base_url = base_url
        self.conversation_history: List[Dict[str, Any]] = []
    
    def chat(self, user_message: str, tools: Optional[List[Dict[str, Any]]] = None,
             system_context: Optional[str] = None, image_base64: Optional[str] = None,
             image_mime: str = "image/jpeg", images_base64: Optional[List[str]] = None) -> Optional[Dict[str, Any]]:
        """Envia mensagem para o modelo com suporte a uma ou múltiplas imagens"""
        try:
            messages = []
            
            if system_context:
                messages.append({"role": "system", "content": system_context})
            
            # Monta mensagem com múltiplas imagens OU uma única imagem
            if images_base64 and len(images_base64) > 0:
                # Múltiplas imagens
                content_parts = []
                # Adiciona todas as imagens primeiro
                for img_b64 in images_base64:
                    content_parts.append({
                        "type": "image_url",
                        "image_url": {"url": f"data:{image_mime};base64,{img_b64}"}
                    })
                # Adiciona texto depois
                if user_message:
                    content_parts.append({"type": "text", "text": user_message})
                
                user_msg = {"role": "user", "content": content_parts}
            elif image_base64:
                # Uma única imagem (compatibilidade)
                user_msg = {
                    "role": "user",
                    "content": [
                        {
                            "type": "image_url",
                            "image_url": {"url": f"data:{image_mime};base64,{image_base64}"}
                        },
                        {
                            "type": "text",
                            "text": user_message
                        }
                    ]
                }
            else:
                # Apenas texto
                user_msg = {"role": "user", "content": user_message}
            
            messages.append(user_msg)
            
            payload = {
                "model": "local-model",
                "messages": messages,
                "temperature": 0.1,
                "max_tokens": 2048,
                "stream": False
            }
            
            response = requests.post(
                f"{self.base_url}/chat/completions",
                json=payload,
                timeout=120
            )
            
            if response.status_code == 200:
                result = response.json()
                # Extrai content do formato correto
                if 'choices' in result and len(result['choices']) > 0:
                    return {"content": result['choices'][0]['message']['content']}
                return result
            else:
                Logger.error(f"Erro do LM Studio: {response.status_code}")
                return None
                
        except Exception as e:
            Logger.error(f"Erro ao comunicar com LM Studio: {e}")
            return None


# ============================================================================
# BATCH ANALYZER V2
# ============================================================================

def batch_analyze_photos_v2(directory: str, pattern: str = "*.{heic,jpg,jpeg,png}") -> Dict[str, Any]:
    """
    Análise em lote com workflow v2.0:
    
    1. Pass 1: CLIP (rápida)
       - Extrai embeddings
       - Detecta features básicas
       - Geocoding
       
    2. Agrupamento
       - Por tempo, localização, pessoas
       
    3. Análise de grupos (LLM)
       - Resume cada grupo
       - Identifica tipo de evento
       
    4. Conclusão global (LLM)
       - Analisa todos os grupos
       - Gera visão geral do acervo
       
    5. Pass 2: LLM por imagem
       - Análise contextualizada
       - Metadados textuais
       - Gera SKP individual
    """
    Logger.info("=" * 80)
    Logger.info("🚀 PhotoGuru Agent v2.0 - Análise Contextual com SKP")
    Logger.info("=" * 80)
    
    # Inicializa
    analyzer = PhotoContextAnalyzer()
    llm_client = LMStudioClient()
    
    # 1. Coleta fotos
    Logger.info("\n📂 Fase 1: Coletando fotos...")
    dir_path = Path(directory)
    extensions = ['heic', 'jpg', 'jpeg', 'png', 'HEIC', 'JPG', 'JPEG', 'PNG']
    
    all_photos = []
    for ext in extensions:
        all_photos.extend(dir_path.glob(f"*.{ext}"))
    
    all_photos = sorted(set(all_photos))
    Logger.success(f"✅ {len(all_photos)} fotos encontradas")
    
    if not all_photos:
        return {"error": "Nenhuma foto encontrada"}
    
    # 2. Pass 1: CLIP Analysis
    Logger.info("\n🔍 Fase 2: Análise CLIP (Pass 1)...")
    
    for i, photo_path in enumerate(all_photos, 1):
        Logger.info(f"   [{i}/{len(all_photos)}] {photo_path.name}")
        
        # Extrai EXIF
        exif = TerminalTools.read_image_metadata(str(photo_path))
        
        # Cria PhotoMetadata
        photo = PhotoMetadata(
            filepath=str(photo_path),
            filename=photo_path.name,
            datetime_original=exif.get('datetime_original'),
            camera_make=exif.get('camera_make'),
            camera_model=exif.get('camera_model'),
            gps_lat=exif.get('gps_lat'),
            gps_lon=exif.get('gps_lon')
        )
        
        # Log data se disponível
        if photo.datetime_original:
            Logger.info(f"      📅 {photo.datetime_original.strftime('%Y-%m-%d %H:%M:%S')}")
        
        # Geocoding
        if photo.gps_lat and photo.gps_lon:
            location = TerminalTools.get_location_from_gps(photo.gps_lat, photo.gps_lon)
            if location:
                photo.location_name = location
                Logger.info(f"      📍 {location}")
        
        # Face detection
        faces = TerminalTools.detect_faces(str(photo_path))
        photo.face_count = faces['face_count']
        photo.face_locations = faces['face_locations']
        if photo.face_count > 0:
            Logger.info(f"      👥 {photo.face_count} face(s)")
        
        # CLIP analysis
        if analyzer.clip_analyzer:
            clip_result = analyzer.clip_analyzer.analyze(str(photo_path))
            photo.clip_embedding = clip_result['embedding']
            photo.clip_features = clip_result['features']
            Logger.info(f"      🏷️  {', '.join(photo.clip_features[:3])}")
        
        # Technical analysis (Pass 1)
        tech_analyzer = TechnicalImageAnalyzer()
        
        # Sharpness (with camera context)
        sharpness_result = tech_analyzer.compute_sharpness(str(photo_path), exif)
        photo.tech_sharpness_score = sharpness_result['overall_score']
        photo.tech_blur_detected = sharpness_result['overall_score'] < 0.3 and not sharpness_result['has_intentional_dof']
        
        # Exposure
        exposure = tech_analyzer.analyze_exposure(str(photo_path))
        photo.tech_exposure_quality = exposure['quality_score']
        photo.tech_highlights_clipped = exposure['highlights_clipped']
        photo.tech_shadows_blocked = exposure['shadows_blocked']
        
        # Aesthetic quality (PyIQA)
        photo.aesthetic_score = tech_analyzer.compute_aesthetic_quality(str(photo_path))
        Logger.info(f"      🎨 Aesthetic: {photo.aesthetic_score:.3f}")
        
        # Technical quality (weighted average)
        technical_quality = (
            0.5 * photo.tech_sharpness_score +
            0.3 * photo.tech_exposure_quality +
            0.2 * (1.0 if photo.face_count and photo.face_count > 0 else 0.5)
        )
        
        # Overall quality combines technical + aesthetic
        photo.tech_overall_quality = (
            0.5 * technical_quality +
            0.5 * photo.aesthetic_score
        )
        
        analyzer.add_photo(photo)
    
    Logger.success("✅ Pass 1 completed")
    
    # 2.5. Interpolate missing locations
    Logger.info("\n📍 Phase 2.5a: Interpolating missing locations...")
    photos_with_gps = sorted(
        [p for p in analyzer.photos if p.gps_lat and p.gps_lon],
        key=lambda x: x.datetime_original if x.datetime_original else datetime.min
    )
    
    for photo in analyzer.photos:
        if photo.datetime_original and not photo.location_name and photo.gps_lat and photo.gps_lon:
            # Find closest photo with location
            closest = None
            min_time_diff = timedelta(hours=999)
            
            for ref in photos_with_gps:
                if ref.location_name and ref.datetime_original:
                    time_diff = abs(photo.datetime_original - ref.datetime_original)
                    if time_diff < min_time_diff:
                        min_time_diff = time_diff
                        closest = ref
            
            if closest and min_time_diff < timedelta(minutes=10):
                photo.location_name = closest.location_name
                Logger.info(f"   ✓ {photo.filename}: {closest.location_name} (interpolated)")
    
    # 2.5b. Duplicate and burst detection
    Logger.info("\n🔍 Phase 2.5b: Detecting duplicates and bursts...")
    
    tech_analyzer = TechnicalImageAnalyzer()
    
    # Compute perceptual hashes
    hashes = {}
    for photo in analyzer.photos:
        hashes[photo.filepath] = tech_analyzer.compute_perceptual_hash(photo.filepath)
    
    # Group duplicates (hamming distance < 5)
    duplicate_groups = {}
    processed = set()
    
    for i, photo1 in enumerate(analyzer.photos):
        if photo1.filepath in processed:
            continue
        
        group = [photo1.filepath]
        hash1 = hashes[photo1.filepath]
        
        for photo2 in analyzer.photos[i+1:]:
            if photo2.filepath in processed:
                continue
            
            hash2 = hashes[photo2.filepath]
            # Hamming distance
            hamming = bin(int(hash1, 16) ^ int(hash2, 16)).count('1')
            
            if hamming < 10:  # Threshold (more permissive)
                group.append(photo2.filepath)
                processed.add(photo2.filepath)
        
        if len(group) > 1:
            group_id = f"dup_{hash1[:8]}"
            duplicate_groups[group_id] = group
            for filepath in group:
                for p in analyzer.photos:
                    if p.filepath == filepath:
                        p.tech_duplicate_group = group_id
                        p.tech_is_duplicate = True
    
    if duplicate_groups:
        Logger.info(f"   📸 {len(duplicate_groups)} grupos de duplicatas encontrados")
    
    # Detect bursts (fotos sequenciais com <5s de diferença e alta similaridade visual)
    burst_groups = {}
    burst_id_counter = 0
    
    photos_sorted = sorted(
        [p for p in analyzer.photos if p.datetime_original],
        key=lambda x: x.datetime_original
    )
    
    i = 0
    while i < len(photos_sorted):
        burst = [photos_sorted[i]]
        j = i + 1
        
        while j < len(photos_sorted):
            time_diff = (photos_sorted[j].datetime_original - burst[-1].datetime_original).total_seconds()
            
            if time_diff > 5:  # Max 5s entre fotos
                break
            
            # Check visual similarity (CLIP embeddings)
            if burst[-1].clip_embedding is not None and photos_sorted[j].clip_embedding is not None:
                sim = np.dot(burst[-1].clip_embedding, photos_sorted[j].clip_embedding)
                if sim > 0.75:  # Similarity threshold (more permissive)
                    burst.append(photos_sorted[j])
                    j += 1
                else:
                    break
            else:
                j += 1
        
        if len(burst) >= 2:  # Minimum 2 photos for burst
            burst_id = f"burst_{burst_id_counter:04d}"
            burst_id_counter += 1
            burst_groups[burst_id] = burst
            
            # Find best photo in burst (highest quality)
            best_photo = max(burst, key=lambda p: p.tech_overall_quality)
            
            for pos, p in enumerate(burst, 1):
                p.tech_burst_group = burst_id
                p.tech_burst_position = pos
                p.tech_is_best_in_burst = (p == best_photo)
            
            i = j
        else:
            i += 1
    
    if burst_groups:
        Logger.info(f"   📸 {len(burst_groups)} bursts detected")
        for burst_id, photos in burst_groups.items():
            Logger.info(f"      {burst_id}: {len(photos)} photos")
    
    # 2.5c. Normalize sharpness scores using percentiles
    Logger.info("\n📊 Phase 2.5c: Normalizing quality scores...")
    sharpness_scores = [p.tech_sharpness_score for p in analyzer.photos]
    if sharpness_scores:
        p25 = np.percentile(sharpness_scores, 25)
        p75 = np.percentile(sharpness_scores, 75)
        
        for photo in analyzer.photos:
            # Normalize to 0-1 based on batch distribution
            if p75 > p25:
                normalized = (photo.tech_sharpness_score - p25) / (p75 - p25)
                photo.tech_sharpness_score = max(0.0, min(1.0, normalized))
                photo.tech_blur_detected = photo.tech_sharpness_score < 0.3
                
                # Recalculate overall quality
                photo.tech_overall_quality = (
                    0.5 * photo.tech_sharpness_score +
                    0.3 * photo.tech_exposure_quality +
                    0.2 * (1.0 if photo.face_count and photo.face_count > 0 else 0.5)
                )
    
    Logger.info(f"   ✓ Sharpness normalized (p25={p25:.2f}, p75={p75:.2f})")
    
    # 3. Grouping
    Logger.info("\n📦 Phase 3: Contextual grouping...")
    groups = analyzer.create_groups()
    
    # 4. Face recognition
    Logger.info("\n👥 Phase 4: Face recognition...")
    face_recognition = TerminalTools.recognize_faces_in_batch(analyzer.photos)
    person_clusters = face_recognition['person_clusters']
    photo_to_persons = face_recognition['photo_to_persons']
    
    if person_clusters:
        Logger.info(f"   Identified {len(person_clusters)} distinct person(s)")
        for person_id, photos in person_clusters.items():
            Logger.info(f"      {person_id}: {len(photos)} photos")
    
    # 5. Pass 2: LLM SEES IMAGES with all metadata
    Logger.info("\n🎯 Phase 5: Individual analysis with images (Pass 2)...")
    
    for i, photo in enumerate(analyzer.photos, 1):
        Logger.info(f"   [{i}/{len(analyzer.photos)}] {photo.filename}")
        
        # Converte imagem para base64
        from PIL import Image
        img = Image.open(photo.filepath)
        
        # Redimensiona
        max_dimension = 1024
        if max(img.size) > max_dimension:
            ratio = max_dimension / max(img.size)
            new_size = tuple(int(dim * ratio) for dim in img.size)
            img = img.resize(new_size, Image.Resampling.LANCZOS)
        
        # Converte para JPEG
        if img.mode in ('RGBA', 'LA', 'P'):
            background = Image.new('RGB', img.size, (255, 255, 255))
            if img.mode == 'P':
                img = img.convert('RGBA')
            background.paste(img, mask=img.split()[-1] if img.mode in ('RGBA', 'LA') else None)
            img = background
        elif img.mode != 'RGB':
            img = img.convert('RGB')
        
        buffer = BytesIO()
        img.save(buffer, format='JPEG', quality=85, optimize=True)
        image_bytes = buffer.getvalue()
        image_base64 = base64.b64encode(image_bytes).decode('utf-8')
        
        # Prepara contexto COM TODOS os metadados disponíveis
        context_parts = []
        
        # Data/hora
        if photo.datetime_original:
            context_parts.append(f"DATA/HORA: {photo.datetime_original.strftime('%Y-%m-%d %H:%M:%S')}")
            context_parts.append(f"  Dia da semana: {photo.datetime_original.strftime('%A')}")
            context_parts.append(f"  Horário: {photo.datetime_original.strftime('%H:%M')}")
        
        # Localização
        if photo.location_name:
            context_parts.append(f"\nLOCALIZAÇÃO GPS: {photo.location_name}")
        
        # Contexto do grupo
        if photo.group_id:
            group = next((g for g in analyzer.groups if g.group_id == photo.group_id), None)
            if group:
                # Informações do grupo
                context_parts.append(f"\nGRUPO/EVENTO:")
                context_parts.append(f"  Tipo: {group.llm_event_type}")
                context_parts.append(f"  Resumo: {group.llm_summary}")
                context_parts.append(f"  Temas: {', '.join(group.llm_common_themes or [])}")
                if group.start_time and group.end_time:
                    context_parts.append(f"  Período: {group.start_time} a {group.end_time}")
                
                # Contexto temporal do grupo
                group_photos = [p for p in analyzer.photos if p.group_id == photo.group_id]
                group_photos.sort(key=lambda x: x.datetime_original or datetime.min)
                
                # Posição da foto no grupo
                photo_position = next((idx for idx, p in enumerate(group_photos, 1) if p.filename == photo.filename), 0)
                context_parts.append(f"  Total de fotos no grupo: {len(group_photos)}")
                context_parts.append(f"  Esta é a foto {photo_position}/{len(group_photos)} na sequência temporal")
                
                # Contexto narrativo simplificado
                if len(group_photos) > 1:
                    first_photo = group_photos[0]
                    last_photo = group_photos[-1]
                    
                    # Informações da primeira foto
                    if photo_position != 1:
                        first_time = first_photo.datetime_original.strftime('%H:%M') if first_photo.datetime_original else 'sem hora'
                        first_loc = first_photo.location_name[:50] if first_photo.location_name else 'sem GPS'
                        first_features = ', '.join(first_photo.clip_features[:2]) if first_photo.clip_features else 'N/A'
                        context_parts.append(f"\n  Primeira foto do grupo ({first_time}): {first_features} em {first_loc}")
                    
                    # Informações da última foto
                    if photo_position != len(group_photos):
                        last_time = last_photo.datetime_original.strftime('%H:%M') if last_photo.datetime_original else 'sem hora'
                        last_loc = last_photo.location_name[:50] if last_photo.location_name else 'sem GPS'
                        last_features = ', '.join(last_photo.clip_features[:2]) if last_photo.clip_features else 'N/A'
                        context_parts.append(f"  Última foto do grupo ({last_time}): {last_features} em {last_loc}")
        
        # Pessoas reconhecidas
        if photo.filename in photo_to_persons:
            persons_in_photo = photo_to_persons[photo.filename]
            context_parts.append(f"\nPESSOAS RECONHECIDAS: {len(persons_in_photo)} pessoa(s) identificada(s)")
            for person_id in persons_in_photo:
                appearances = len(person_clusters.get(person_id, []))
                context_parts.append(f"  - {person_id}: aparece em {appearances} fotos do acervo")
        elif photo.face_count and photo.face_count > 0:
            context_parts.append(f"\nPESSOAS: {photo.face_count} face(s) detectada(s) (não reconhecidas)")
        
        # Features CLIP
        if photo.clip_features:
            context_parts.append(f"\nFeatures visuais detectadas: {', '.join(photo.clip_features)}")
        
        context_text = "\n".join(context_parts)
        
        # Request contextualized analysis from LLM WITH IMAGE
        prompt = f"""OBSERVE THIS IMAGE carefully and analyze considering the context:

{context_text}

INSTRUCTIONS:
1. LOOK AT THE IMAGE: Describe ONLY what you SEE - people, objects, actions, scenery
2. UNDERSTAND CONTEXT: This photo is part of a temporal/spatial SEQUENCE
3. CREATE NARRATIVE: Connect this scene with the larger event (use time/location/position in sequence)
4. BE SPECIFIC: Mention concrete visual details you observe
5. NO MARKDOWN: DO NOT use **, ##, or other markers
6. NO FABRICATION: Do NOT mention things you don't see. Do NOT invent markets, stores, or details.

EXAMPLE OF GOOD DESCRIPTION:
"Car interior in motion, showing dashboard with clock at 13:25. Through windshield, residential street in Salvador heading to Bonfim. This is the first photo in the sequence, capturing the START of the journey to Igreja do Bonfim."

Provide metadata:

TITLE: [4-8 contextual descriptive words]

DESCRIPTION: [2-3 sentences about what you SEE and how it connects to the temporal/spatial narrative]

KEYWORDS: [15-20 English words, comma-separated, NO dates or times]

CATEGORY: [people/landscape/architecture/food/technology/event/document/art/wildlife/sports/other]

SCENE: [indoor/outdoor/aerial/underwater/studio/street/nature/urban]

MOOD: [2-3 words about atmosphere]"""
        
        response = llm_client.chat(
            prompt, 
            system_context="You are an expert in contextual photo analysis. Analyze the IMAGE observing specific visual details and connect with the temporal sequence context. Do NOT fabricate or mention things not visible in the image.",
            image_base64=image_base64,
            image_mime="image/jpeg"
        )
        
        if response and 'content' in response:
            content = response['content']
            
            try:
                # Parse resposta estruturada
                title_match = re.search(r'TITLE:\s*(.+?)(?=\n\n|\nDESCRIPTION:|$)', content, re.IGNORECASE | re.DOTALL)
                desc_match = re.search(r'DESCRIPTION:\s*(.+?)(?=\n\n|\nKEYWORDS:|$)', content, re.IGNORECASE | re.DOTALL)
                keywords_match = re.search(r'KEYWORDS:\s*(.+?)(?=\n\n|\nCATEGORY:|$)', content, re.IGNORECASE | re.DOTALL)
                category_match = re.search(r'CATEGORY:\s*(.+?)(?=\n\n|\nSCENE:|$)', content, re.IGNORECASE | re.DOTALL)
                scene_match = re.search(r'SCENE:\s*(.+?)(?=\n\n|\nMOOD:|$)', content, re.IGNORECASE | re.DOTALL)
                mood_match = re.search(r'MOOD:\s*(.+?)(?=\n|$)', content, re.IGNORECASE | re.DOTALL)
                
                # Parse and clean markdown formatting
                photo.llm_title = title_match.group(1).strip() if title_match else "Untitled"
                photo.llm_description = desc_match.group(1).strip() if desc_match else ""
                raw_keywords = [k.strip() for k in keywords_match.group(1).split(',') if k.strip()] if keywords_match else []
                
                # Filter out dates, times, and clean keywords
                filtered_keywords = []
                for k in raw_keywords:
                    k_clean = k.strip('*').strip().lower()
                    # Skip dates (contains numbers and months/years)
                    if any(month in k_clean for month in ['january', 'february', 'march', 'april', 'may', 'june', 'july', 'august', 'september', 'october', 'november', 'december']):
                        continue
                    if re.search(r'\d{4}|\d{1,2}(st|nd|rd|th)', k_clean):  # Years or dates
                        continue
                    if k_clean and len(k_clean) > 2:  # Keep meaningful keywords
                        filtered_keywords.append(k)
                
                photo.llm_keywords = filtered_keywords[:20]  # Max 20
                photo.llm_category = category_match.group(1).strip() if category_match else "other"
                photo.llm_scene = scene_match.group(1).strip() if scene_match else "unknown"
                
                # Remove markdown indevido (**, ##, etc)
                if photo.llm_title.startswith('**'):
                    photo.llm_title = photo.llm_title.strip('*').strip()
                if photo.llm_description.startswith('**'):
                    photo.llm_description = photo.llm_description.strip('*').strip()
                if photo.llm_category.startswith('**'):
                    photo.llm_category = photo.llm_category.strip('*').strip()
                
                # Limpa keywords
                photo.llm_keywords = [k.strip('*').strip() for k in photo.llm_keywords]
                photo.llm_mood = mood_match.group(1).strip() if mood_match else ""
                
                Logger.info(f"      ✓ {photo.llm_title}")
            except Exception as e:
                Logger.warning(f"      ⚠️ Failed to parse metadata: {e}")
        
        # Create image SKP
        if photo.clip_embedding is not None:
            photo.skp_image = SemanticKey(
                vector=photo.clip_embedding,
                key_id=hashlib.md5(photo.filename.encode()).hexdigest()[:8],
                role="anchor",
                metadata={
                    "type": "image",
                    "filename": photo.filename,
                    "title": photo.llm_title
                }
            )
        
        # Reference to global keys (will be set later)
        photo.skp_global_key = "global"
    
    Logger.success("✅ Pass 2 completed")
    
    # 6. Group analysis with LLM
    Logger.info("\n📊 Phase 6: Contextual group analysis...")
    analyzer.analyze_groups_with_llm(llm_client)
    
    # 7. Conclusão global
    Logger.info("\n🌍 Fase 7: Criando conclusão global...")
    conclusion = analyzer.create_global_conclusion(llm_client)
    
    # 8. Criar SKP keys
    Logger.info("\n🔑 Fase 8: Criando Semantic Keys (SKP)...")
    analyzer.create_skp_keys()
    
    # 9. Salva resultados
    Logger.info("\n💾 Fase 9: Salvando resultados...")
    
    output = {
        "total_photos": len(analyzer.photos),
        "groups_count": len(analyzer.groups),
        "global_conclusion": {
            "summary": conclusion.llm_overall_summary,
            "themes": conclusion.llm_main_themes,
            "locations": conclusion.llm_main_locations,
            "social_patterns": conclusion.llm_social_patterns
        },
        "groups": [],
        "photos": []
    }
    
    # Grupos
    for group in analyzer.groups:
        output['groups'].append({
            "group_id": group.group_id,
            "photos_count": len(group.photos),
            "event_type": group.llm_event_type,
            "summary": group.llm_summary,
            "themes": group.llm_common_themes,
            "skp_key_id": group.skp_group_key.key_id if group.skp_group_key else None
        })
    
    # Fotos
    for photo in analyzer.photos:
        photo_data = {
            "filename": photo.filename,
            "title": photo.llm_title,
            "description": photo.llm_description,
            "keywords": photo.llm_keywords,
            "category": photo.llm_category,
            "location": photo.location_name,
            "group_id": photo.group_id,
            "skp_image_key": photo.skp_image.key_id if photo.skp_image else None,
            "skp_group_keys": photo.skp_group_keys,
            "skp_global_key": photo.skp_global_key,
            
            # Technical metadata
            "technical": {
                "sharpness_score": round(photo.tech_sharpness_score, 3),
                "blur_detected": photo.tech_blur_detected,
                "overall_quality": round(photo.tech_overall_quality, 3),
                "exposure_quality": round(photo.tech_exposure_quality, 3),
                "highlights_clipped": photo.tech_highlights_clipped,
                "shadows_blocked": photo.tech_shadows_blocked,
            }
        }
        
        # Add duplicate info if exists
        if photo.tech_duplicate_group:
            photo_data["technical"]["duplicate_group"] = photo.tech_duplicate_group
            photo_data["technical"]["is_duplicate"] = photo.tech_is_duplicate
        
        # Add burst info if exists
        if photo.tech_burst_group:
            photo_data["technical"]["burst_group"] = photo.tech_burst_group
            photo_data["technical"]["burst_position"] = photo.tech_burst_position
            photo_data["technical"]["is_best_in_burst"] = photo.tech_is_best_in_burst
        
        # Add face quality if available
        if photo.tech_face_quality_score is not None:
            photo_data["technical"]["face_quality_score"] = round(photo.tech_face_quality_score, 3)
            if photo.tech_face_eyes_open is not None:
                photo_data["technical"]["face_eyes_open"] = photo.tech_face_eyes_open
            if photo.tech_face_smiling is not None:
                photo_data["technical"]["face_smiling"] = photo.tech_face_smiling
        
        output['photos'].append(photo_data)
    
    # Salva JSON
    output_file = dir_path / "photoguru_analysis_v2.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(output, f, indent=2, ensure_ascii=False, default=str)
    
    Logger.success(f"✅ Análise salva em: {output_file}")
    
    Logger.info("\n" + "=" * 80)
    Logger.info("✨ Análise concluída!")
    Logger.info("=" * 80)
    
    return output


# ============================================================================
# CLI COMMANDS
# ============================================================================

def cmd_info(photo_path: str):
    """Show detailed info about a single photo"""
    Logger.info(f"📸 Photo Info: {photo_path}")
    
    # EXIF
    exif = TerminalTools.read_image_metadata(photo_path)
    Logger.info("\n📋 EXIF Metadata:")
    for key, value in exif.items():
        Logger.info(f"   {key}: {value}")
    
    # Technical analysis
    Logger.info("\n🔍 Technical Analysis:")
    tech = TechnicalImageAnalyzer()
    sharpness = tech.compute_sharpness(photo_path, exif)
    exposure = tech.analyze_exposure(photo_path)
    phash = tech.compute_perceptual_hash(photo_path)
    
    Logger.info(f"   Overall sharpness: {sharpness['overall_score']:.3f}")
    Logger.info(f"   Global sharpness: {sharpness['global_sharpness']:.3f}")
    Logger.info(f"   Max local sharpness: {sharpness['max_local_sharpness']:.3f}")
    Logger.info(f"   Intentional depth-of-field: {'Yes' if sharpness['has_intentional_dof'] else 'No'}")
    Logger.info(f"   Motion blur risk: {'Yes' if sharpness['motion_blur_risk'] else 'No'}")
    if sharpness['analysis_notes']:
        Logger.info(f"   Camera context analysis:")
        for note in sharpness['analysis_notes']:
            Logger.info(f"      • {note}")
    Logger.info(f"   Exposure quality: {exposure['quality_score']:.3f}")
    Logger.info(f"   Highlights clipped: {exposure['highlights_clipped']}")
    Logger.info(f"   Shadows blocked: {exposure['shadows_blocked']}")
    Logger.info(f"   Perceptual hash: {phash}")
    
    # Aesthetic quality
    aesthetic = tech.compute_aesthetic_quality(photo_path)
    Logger.info(f"\n🎨 Aesthetic Quality:")
    Logger.info(f"   Aesthetic score: {aesthetic:.3f}")
    Logger.info(f"   (MUSIQ-AVA: trained on 250K human-rated photos)")
    
    # CLIP
    if CLIP_AVAILABLE:
        Logger.info("\n🏷️ CLIP Features:")
        clip_analyzer = CLIPAnalyzer()
        result = clip_analyzer.analyze(photo_path)
        for feature, score in list(result['scores'].items())[:10]:
            Logger.info(f"   {feature}: {score:.3f}")


def cmd_duplicates(directory: str, threshold: int = 10):
    """Find duplicate photos in directory"""
    Logger.info(f"🔍 Finding duplicates in: {directory}")
    
    dir_path = Path(directory)
    extensions = ['heic', 'jpg', 'jpeg', 'png', 'HEIC', 'JPG', 'JPEG', 'PNG']
    
    all_photos = []
    for ext in extensions:
        all_photos.extend(dir_path.glob(f"*.{ext}"))
    
    Logger.info(f"   Found {len(all_photos)} photos")
    
    tech = TechnicalImageAnalyzer()
    hashes = {}
    
    Logger.info("   Computing perceptual hashes...")
    for photo in all_photos:
        hashes[str(photo)] = tech.compute_perceptual_hash(str(photo))
    
    # Find duplicates
    duplicate_groups = []
    processed = set()
    
    for i, photo1 in enumerate(all_photos):
        if str(photo1) in processed:
            continue
        
        group = [photo1]
        hash1 = hashes[str(photo1)]
        
        for photo2 in all_photos[i+1:]:
            if str(photo2) in processed:
                continue
            
            hash2 = hashes[str(photo2)]
            hamming = bin(int(hash1, 16) ^ int(hash2, 16)).count('1')
            
            if hamming < threshold:
                group.append(photo2)
                processed.add(str(photo2))
        
        if len(group) > 1:
            duplicate_groups.append(group)
    
    if duplicate_groups:
        Logger.info(f"\n✅ Found {len(duplicate_groups)} duplicate groups:")
        for i, group in enumerate(duplicate_groups, 1):
            Logger.info(f"\n   Group {i} ({len(group)} photos):")
            for photo in group:
                Logger.info(f"      - {photo.name}")
    else:
        Logger.info("\n✅ No duplicates found")


def cmd_bursts(directory: str, max_seconds: int = 5, min_photos: int = 2):
    """Find burst sequences in directory"""
    Logger.info(f"📸 Finding bursts in: {directory}")
    
    dir_path = Path(directory)
    extensions = ['heic', 'jpg', 'jpeg', 'png', 'HEIC', 'JPG', 'JPEG', 'PNG']
    
    all_photos = []
    for ext in extensions:
        all_photos.extend(dir_path.glob(f"*.{ext}"))
    
    # Load EXIF
    photos_with_time = []
    for photo in all_photos:
        exif = TerminalTools.read_image_metadata(str(photo))
        if exif.get('datetime_original'):
            photos_with_time.append({
                'path': photo,
                'time': exif['datetime_original']
            })
    
    photos_with_time.sort(key=lambda x: x['time'])
    Logger.info(f"   Found {len(photos_with_time)} photos with timestamps")
    
    # Find bursts
    bursts = []
    i = 0
    while i < len(photos_with_time):
        burst = [photos_with_time[i]]
        j = i + 1
        
        while j < len(photos_with_time):
            time_diff = (photos_with_time[j]['time'] - burst[-1]['time']).total_seconds()
            if time_diff <= max_seconds:
                burst.append(photos_with_time[j])
                j += 1
            else:
                break
        
        if len(burst) >= min_photos:
            bursts.append(burst)
            i = j
        else:
            i += 1
    
    if bursts:
        Logger.info(f"\n✅ Found {len(bursts)} bursts:")
        for i, burst in enumerate(bursts, 1):
            duration = (burst[-1]['time'] - burst[0]['time']).total_seconds()
            Logger.info(f"\n   Burst {i} ({len(burst)} photos, {duration:.1f}s):")
            for photo in burst:
                Logger.info(f"      - {photo['path'].name} ({photo['time'].strftime('%H:%M:%S')})")
    else:
        Logger.info("\n✅ No bursts found")


def cmd_quality(directory: str, sort_by: str = 'overall'):
    """Analyze and rank photos by quality"""
    Logger.info(f"⭐ Quality analysis: {directory}")
    
    dir_path = Path(directory)
    extensions = ['heic', 'jpg', 'jpeg', 'png', 'HEIC', 'JPG', 'JPEG', 'PNG']
    
    all_photos = []
    for ext in extensions:
        all_photos.extend(dir_path.glob(f"*.{ext}"))
    
    Logger.info(f"   Analyzing {len(all_photos)} photos...")
    
    tech = TechnicalImageAnalyzer()
    results = []
    
    for i, photo in enumerate(all_photos, 1):
        Logger.info(f"   [{i}/{len(all_photos)}] {photo.name}...")
        exif = TerminalTools.read_image_metadata(str(photo))
        sharpness_result = tech.compute_sharpness(str(photo), exif)
        exposure = tech.analyze_exposure(str(photo))
        aesthetic = tech.compute_aesthetic_quality(str(photo))
        
        # Detect faces
        faces = TerminalTools.detect_faces(str(photo))
        face_bonus = 0.2 if faces['face_count'] > 0 else 0.0
        
        # Technical quality
        technical = 0.5 * sharpness_result['overall_score'] + 0.3 * exposure['quality_score'] + face_bonus
        
        # Overall quality = aesthetic only (technical is just informative)
        overall = aesthetic
        
        results.append({
            'name': photo.name,
            'sharpness': sharpness_result['overall_score'],
            'exposure': exposure['quality_score'],
            'aesthetic': aesthetic,
            'technical': technical,
            'overall': overall,
            'blur': sharpness_result['overall_score'] < 0.3,
            'clipped': exposure['highlights_clipped'],
            'faces': faces['face_count']
        })
    
    # Sort
    sort_key = sort_by if sort_by in ['sharpness', 'exposure', 'overall', 'aesthetic', 'technical'] else 'overall'
    results.sort(key=lambda x: x[sort_key], reverse=True)
    
    Logger.info(f"\n✅ Quality ranking (sorted by {sort_key}):\n")
    Logger.info(f"{'Rank':<6} {'Photo':<30} {'Overall':<10} {'Tech':<8} {'Aesth':<8} {'Faces':<6} {'Issues'}")
    Logger.info("-" * 90)
    
    for i, r in enumerate(results, 1):
        issues = []
        if r['blur']: issues.append('blur')
        if r['clipped']: issues.append('clipped')
        
        Logger.info(f"{i:<6} {r['name']:<30} {r['overall']:.3f}      {r['technical']:.3f}    {r['aesthetic']:.3f}    {r['faces']:<6} {', '.join(issues)}")


def cmd_compare(photo1: str, photo2: str):
    """Compare two photos"""
    Logger.info(f"⚖️ Comparing photos:")
    Logger.info(f"   A: {photo1}")
    Logger.info(f"   B: {photo2}")
    
    tech = TechnicalImageAnalyzer()
    
    # Photo A
    Logger.info("\n📸 Photo A:")
    sharp_a_result = tech.compute_sharpness(photo1)
    expo_a = tech.analyze_exposure(photo1)
    hash_a = tech.compute_perceptual_hash(photo1)
    Logger.info(f"   Sharpness: {sharp_a_result['overall_score']:.3f}")
    if sharp_a_result['has_intentional_dof']:
        Logger.info(f"   (Intentional DoF: global={sharp_a_result['global_sharpness']:.3f}, max_local={sharp_a_result['max_local_sharpness']:.3f})")
    Logger.info(f"   Exposure: {expo_a['quality_score']:.3f}")
    Logger.info(f"   Hash: {hash_a}")
    
    # Photo B
    Logger.info("\n📸 Photo B:")
    sharp_b_result = tech.compute_sharpness(photo2)
    expo_b = tech.analyze_exposure(photo2)
    hash_b = tech.compute_perceptual_hash(photo2)
    Logger.info(f"   Sharpness: {sharp_b_result['overall_score']:.3f}")
    if sharp_b_result['has_intentional_dof']:
        Logger.info(f"   (Intentional DoF: global={sharp_b_result['global_sharpness']:.3f}, max_local={sharp_b_result['max_local_sharpness']:.3f})")
    Logger.info(f"   Exposure: {expo_b['quality_score']:.3f}")
    Logger.info(f"   Hash: {hash_b}")
    
    sharp_a = sharp_a_result['overall_score']
    sharp_b = sharp_b_result['overall_score']
    
    # Comparison
    Logger.info("\n🔍 Comparison:")
    hamming = bin(int(hash_a, 16) ^ int(hash_b, 16)).count('1')
    Logger.info(f"   Perceptual similarity: {100 - (hamming / 64 * 100):.1f}%")
    Logger.info(f"   Duplicate: {'YES' if hamming < 10 else 'NO'}")
    
    if sharp_a > sharp_b:
        Logger.info(f"   ✓ Photo A is sharper (+{(sharp_a - sharp_b):.3f})")
    else:
        Logger.info(f"   ✓ Photo B is sharper (+{(sharp_b - sharp_a):.3f})")
    
    if expo_a['quality_score'] > expo_b['quality_score']:
        Logger.info(f"   ✓ Photo A has better exposure (+{(expo_a['quality_score'] - expo_b['quality_score']):.3f})")
    else:
        Logger.info(f"   ✓ Photo B has better exposure (+{(expo_b['quality_score'] - expo_a['quality_score']):.3f})")


def cmd_test(component: str = 'all'):
    """Test individual components"""
    Logger.info(f"🧪 Testing component: {component}")
    
    if component in ['clip', 'all']:
        Logger.info("\n🔍 Testing CLIP...")
        if CLIP_AVAILABLE:
            clip = CLIPAnalyzer()
            Logger.success("   ✅ CLIP initialized")
        else:
            Logger.error("   ❌ CLIP not available")
    
    if component in ['face', 'all']:
        Logger.info("\n👤 Testing Face Detection...")
        if FACE_RECOGNITION_AVAILABLE:
            Logger.success("   ✅ OpenCV available")
        else:
            Logger.error("   ❌ OpenCV not available")
    
    if component in ['llm', 'all']:
        Logger.info("\n🤖 Testing LLM connection...")
        try:
            llm = LMStudioClient()
            response = llm.chat("Say 'test' if you can read this", system_context="You are a test assistant")
            if response:
                Logger.success("   ✅ LLM connected")
            else:
                Logger.error("   ❌ LLM not responding")
        except Exception as e:
            Logger.error(f"   ❌ LLM error: {e}")
    
    if component in ['technical', 'all']:
        Logger.info("\n⚙️ Testing Technical Analysis...")
        try:
            tech = TechnicalImageAnalyzer()
            Logger.success("   ✅ TechnicalImageAnalyzer initialized")
        except Exception as e:
            Logger.error(f"   ❌ Error: {e}")


def cmd_write(json_path: str, photo_index: Optional[int] = None):
    """Write metadata from JSON analysis back to photo files"""
    Logger.info(f"📝 Writing metadata from: {json_path}")
    
    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        photos = data.get('photos', [])
        if not photos:
            Logger.error("No photos found in JSON")
            return
        
        if photo_index is not None:
            if photo_index < 0 or photo_index >= len(photos):
                Logger.error(f"Invalid index {photo_index}. Valid range: 0-{len(photos)-1}")
                return
            photos_to_write = [photos[photo_index]]
        else:
            photos_to_write = photos
        
        Logger.info(f"Writing metadata to {len(photos_to_write)} photo(s)...")
        
        # Get base directory from JSON path
        json_dir = Path(json_path).parent
        
        success_count = 0
        for photo_dict in photos_to_write:
            # Build filepath
            filename = photo_dict.get('filename')
            filepath = str(json_dir / filename) if filename else None
            
            if not filepath or not Path(filepath).exists():
                Logger.warning(f"File not found: {filepath}")
                continue
            
            # Extract technical metadata
            tech = photo_dict.get('technical', {})
            
            # Convert dict to PhotoMetadata
            photo = PhotoMetadata(
                filepath=filepath,
                filename=filename,
                llm_title=photo_dict.get('title'),
                llm_description=photo_dict.get('description'),
                llm_keywords=photo_dict.get('keywords', []),
                llm_category=photo_dict.get('category'),
                llm_scene=photo_dict.get('scene'),
                llm_mood=photo_dict.get('mood'),
                location_name=photo_dict.get('location'),
                tech_sharpness_score=tech.get('sharpness_score', 0),
                tech_exposure_quality=tech.get('exposure_quality', 0),
                aesthetic_score=tech.get('aesthetic_score', 0),
                tech_overall_quality=tech.get('overall_quality', 0),
                tech_duplicate_group=tech.get('duplicate_group'),
                tech_burst_group=tech.get('burst_group'),
                tech_burst_position=tech.get('burst_position'),
                tech_is_best_in_burst=tech.get('is_best_in_burst', False),
                face_count=tech.get('face_count', 0)
            )
            
            Logger.info(f"\n   Writing to: {filename}")
            if TerminalTools.write_image_metadata(photo):
                success_count += 1
        
        Logger.success(f"\n✅ Successfully wrote metadata to {success_count}/{len(photos_to_write)} photos")
        
    except FileNotFoundError:
        Logger.error(f"JSON file not found: {json_path}")
    except json.JSONDecodeError as e:
        Logger.error(f"Invalid JSON: {e}")
    except Exception as e:
        Logger.error(f"Error: {e}")
        traceback.print_exc()


# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='PhotoGuru AI Agent v2.0 - Contextual photo analysis with SKP',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Commands:
  batch      Full analysis of a directory
  info       Show detailed info about a single photo
  duplicates Find duplicate photos in directory
  bursts     Find burst sequences in directory
  quality    Analyze and rank photos by quality
  compare    Compare two photos
  write      Write metadata from JSON to photo files
  test       Test individual components

Examples:
  python agent_v2.py batch Test_10/
  python agent_v2.py info Test_10/IMG_0001.HEIC
  python agent_v2.py duplicates Test_10/
  python agent_v2.py bursts Test_10/ --max-seconds 10
  python agent_v2.py quality Test_10/ --sort-by sharpness
  python agent_v2.py compare photo1.jpg photo2.jpg
  python agent_v2.py write Test_10/photoguru_analysis_v2.json
  python agent_v2.py write Test_10/photoguru_analysis_v2.json --index 0
  python agent_v2.py test clip
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    
    # batch
    batch_parser = subparsers.add_parser('batch', help='Full analysis of directory')
    batch_parser.add_argument('directory', help='Directory containing photos')
    batch_parser.add_argument('-p', '--pattern', default='*.{heic,jpg,jpeg,png}',
                             help='File pattern (default: *.{heic,jpg,jpeg,png})')
    
    # info
    info_parser = subparsers.add_parser('info', help='Show photo info')
    info_parser.add_argument('photo', help='Photo path')
    
    # duplicates
    dup_parser = subparsers.add_parser('duplicates', help='Find duplicates')
    dup_parser.add_argument('directory', help='Directory to scan')
    dup_parser.add_argument('-t', '--threshold', type=int, default=10,
                           help='Hamming distance threshold (default: 10)')
    
    # bursts
    burst_parser = subparsers.add_parser('bursts', help='Find burst sequences')
    burst_parser.add_argument('directory', help='Directory to scan')
    burst_parser.add_argument('-s', '--max-seconds', type=int, default=5,
                             help='Max seconds between photos (default: 5)')
    burst_parser.add_argument('-m', '--min-photos', type=int, default=2,
                             help='Min photos in burst (default: 2)')
    
    # quality
    quality_parser = subparsers.add_parser('quality', help='Analyze quality')
    quality_parser.add_argument('directory', help='Directory to analyze')
    quality_parser.add_argument('-s', '--sort-by', choices=['overall', 'sharpness', 'exposure'],
                               default='overall', help='Sort criterion (default: overall)')
    
    # compare
    compare_parser = subparsers.add_parser('compare', help='Compare two photos')
    compare_parser.add_argument('photo1', help='First photo')
    compare_parser.add_argument('photo2', help='Second photo')
    
    # test
    test_parser = subparsers.add_parser('test', help='Test components')
    test_parser.add_argument('component', nargs='?', default='all',
                            choices=['all', 'clip', 'face', 'llm', 'technical'],
                            help='Component to test (default: all)')
    
    # write
    write_parser = subparsers.add_parser('write', help='Write metadata from JSON to photos')
    write_parser.add_argument('json_path', help='Path to photoguru_analysis_v2.json')
    write_parser.add_argument('-i', '--index', type=int, dest='photo_index',
                             help='Write metadata for specific photo index only (default: all)')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        sys.exit(1)
    
    try:
        if args.command == 'batch':
            result = batch_analyze_photos_v2(args.directory, args.pattern)
            if 'error' in result:
                Logger.error(result['error'])
                sys.exit(1)
        
        elif args.command == 'info':
            cmd_info(args.photo)
        
        elif args.command == 'duplicates':
            cmd_duplicates(args.directory, args.threshold)
        
        elif args.command == 'bursts':
            cmd_bursts(args.directory, args.max_seconds, args.min_photos)
        
        elif args.command == 'quality':
            cmd_quality(args.directory, args.sort_by)
        
        elif args.command == 'write':
            cmd_write(args.json_path, args.photo_index)
        
        elif args.command == 'compare':
            cmd_compare(args.photo1, args.photo2)
        
        elif args.command == 'test':
            cmd_test(args.component)
            
    except KeyboardInterrupt:
        print("\n\n👋 Interrupted by user")
    except Exception as e:
        Logger.error(f"Unexpected error: {e}")
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
