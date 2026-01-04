#!/usr/bin/env python3
"""
PhotoGuru MVP Agent - Simplified Photo Analysis
Cloud-based AI analysis without heavy local dependencies
"""

import os
import sys
import json
import base64
import argparse
from pathlib import Path
from typing import Dict, Any, Optional, List
from datetime import datetime

try:
    from PIL import Image
    import requests
    PIL_AVAILABLE = True
except ImportError:
    print("⚠️ Please install: pip install Pillow requests")
    PIL_AVAILABLE = False

# EXIF/XMP support
try:
    import exiftool
    EXIFTOOL_AVAILABLE = True
except ImportError:
    EXIFTOOL_AVAILABLE = False
    print("⚠️ exiftool not available. Metadata writing will be limited.")


class PhotoAnalyzer:
    """Simplified photo analyzer using cloud APIs"""
    
    def __init__(self, api_key: Optional[str] = None, provider: str = "openai"):
        """
        Initialize analyzer
        
        Args:
            api_key: API key for cloud service (OpenAI, Anthropic, etc.)
            provider: "openai" or "anthropic"
        """
        self.api_key = api_key or os.getenv("OPENAI_API_KEY") or os.getenv("ANTHROPIC_API_KEY")
        self.provider = provider
        
        if not self.api_key:
            print("⚠️ No API key found. Set OPENAI_API_KEY or ANTHROPIC_API_KEY environment variable")
    
    def analyze_photo(self, filepath: str) -> Optional[Dict[str, Any]]:
        """
        Analyze photo and return structured metadata
        
        Returns:
            {
                'title': str,
                'description': str,
                'tags': List[str],
                'subjects': List[str],
                'scene_type': str,
                'confidence': float
            }
        """
        if not PIL_AVAILABLE or not self.api_key:
            return None
        
        try:
            # Load and encode image
            image = Image.open(filepath)
            
            # Resize for API efficiency (max 1024px)
            max_size = 1024
            if max(image.size) > max_size:
                ratio = max_size / max(image.size)
                new_size = tuple(int(dim * ratio) for dim in image.size)
                image = image.resize(new_size, Image.Resampling.LANCZOS)
            
            # Convert to base64
            from io import BytesIO
            buffer = BytesIO()
            image.save(buffer, format="JPEG", quality=85)
            image_b64 = base64.b64encode(buffer.getvalue()).decode()
            
            # Call API
            if self.provider == "openai":
                result = self._analyze_openai(image_b64)
            else:
                result = self._analyze_anthropic(image_b64)
            
            return result
            
        except Exception as e:
            print(f"❌ Error analyzing {filepath}: {e}")
            return None
    
    def _analyze_openai(self, image_b64: str) -> Dict[str, Any]:
        """Analyze using OpenAI GPT-4 Vision"""
        
        prompt = """Analyze this photo and provide:
1. A concise, descriptive title (max 60 chars)
2. A detailed description (2-3 sentences)
3. 5-10 relevant tags/keywords
4. Main subjects in the photo
5. Scene type (landscape, portrait, street, indoor, etc.)

Return as JSON:
{
  "title": "...",
  "description": "...",
  "tags": ["tag1", "tag2", ...],
  "subjects": ["subject1", "subject2", ...],
  "scene_type": "..."
}"""
        
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {self.api_key}"
        }
        
        payload = {
            "model": "gpt-4-vision-preview",
            "messages": [
                {
                    "role": "user",
                    "content": [
                        {"type": "text", "text": prompt},
                        {
                            "type": "image_url",
                            "image_url": {
                                "url": f"data:image/jpeg;base64,{image_b64}"
                            }
                        }
                    ]
                }
            ],
            "max_tokens": 500
        }
        
        response = requests.post(
            "https://api.openai.com/v1/chat/completions",
            headers=headers,
            json=payload,
            timeout=30
        )
        
        if response.status_code != 200:
            print(f"❌ API Error: {response.status_code} - {response.text}")
            return self._fallback_analysis()
        
        result = response.json()
        content = result['choices'][0]['message']['content']
        
        # Extract JSON from response
        try:
            # Try to parse as JSON directly
            analysis = json.loads(content)
        except json.JSONDecodeError:
            # Try to extract JSON from markdown code block
            import re
            json_match = re.search(r'```json\s*(\{.*?\})\s*```', content, re.DOTALL)
            if json_match:
                analysis = json.loads(json_match.group(1))
            else:
                print(f"⚠️ Could not parse API response: {content}")
                return self._fallback_analysis()
        
        analysis['confidence'] = 0.9
        return analysis
    
    def _analyze_anthropic(self, image_b64: str) -> Dict[str, Any]:
        """Analyze using Anthropic Claude"""
        # Similar implementation for Claude API
        # For MVP, we'll focus on OpenAI
        return self._fallback_analysis()
    
    def _fallback_analysis(self) -> Dict[str, Any]:
        """Fallback analysis when API fails"""
        return {
            'title': 'Untitled Photo',
            'description': 'Photo awaiting analysis',
            'tags': [],
            'subjects': [],
            'scene_type': 'unknown',
            'confidence': 0.0
        }
    
    def write_metadata(self, filepath: str, metadata: Dict[str, Any]) -> bool:
        """
        Write metadata to image file using XMP/IPTC
        
        Args:
            filepath: Path to image file
            metadata: Metadata dict with title, description, tags, etc.
        
        Returns:
            True if successful
        """
        if not EXIFTOOL_AVAILABLE:
            print("⚠️ exiftool not available, cannot write metadata")
            return False
        
        try:
            with exiftool.ExifTool() as et:
                # Build exiftool arguments
                args = []
                
                if 'title' in metadata:
                    args.extend(['-XMP:Title=' + metadata['title']])
                    args.extend(['-IPTC:ObjectName=' + metadata['title']])
                
                if 'description' in metadata:
                    args.extend(['-XMP:Description=' + metadata['description']])
                    args.extend(['-IPTC:Caption-Abstract=' + metadata['description']])
                
                if 'tags' in metadata and metadata['tags']:
                    for tag in metadata['tags']:
                        args.extend(['-XMP:Subject+=' + tag])
                        args.extend(['-IPTC:Keywords+=' + tag])
                
                if 'subjects' in metadata:
                    subjects_str = ', '.join(metadata['subjects'])
                    args.extend(['-XMP:PersonInImage=' + subjects_str])
                
                # Add custom namespace for PhotoGuru
                if 'confidence' in metadata:
                    args.extend(['-XMP:PhotoGuruConfidence=' + str(metadata['confidence'])])
                
                args.extend(['-XMP:DateAnalyzed=' + datetime.now().isoformat()])
                
                # Write metadata
                args.append(filepath)
                et.execute(*args)
                
                return True
                
        except Exception as e:
            print(f"❌ Error writing metadata to {filepath}: {e}")
            return False


class SimpleSearch:
    """Simple search in photo metadata"""
    
    def __init__(self, directory: str):
        self.directory = Path(directory)
    
    def search(self, query: str) -> List[str]:
        """
        Search for photos matching query
        
        Args:
            query: Search terms (space-separated)
        
        Returns:
            List of matching file paths
        """
        query_terms = query.lower().split()
        results = []
        
        for filepath in self.directory.rglob("*.jpg"):
            # Read metadata
            try:
                with exiftool.ExifTool() as et:
                    metadata = et.get_metadata(str(filepath))
                    
                    # Search in various fields
                    searchable_text = ' '.join([
                        metadata.get('XMP:Title', ''),
                        metadata.get('XMP:Description', ''),
                        ' '.join(metadata.get('XMP:Subject', [])),
                        metadata.get('IPTC:Keywords', ''),
                    ]).lower()
                    
                    # Check if all query terms are present
                    if all(term in searchable_text for term in query_terms):
                        results.append(str(filepath))
                        
            except Exception:
                continue
        
        return results


def main():
    """CLI interface for MVP agent"""
    parser = argparse.ArgumentParser(description="PhotoGuru MVP Agent")
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Analyze command
    analyze_parser = subparsers.add_parser('analyze', help='Analyze a photo')
    analyze_parser.add_argument('file', help='Photo file to analyze')
    analyze_parser.add_argument('--write', action='store_true', help='Write metadata to file')
    analyze_parser.add_argument('--api-key', help='API key (or use OPENAI_API_KEY env var)')
    
    # Batch analyze command
    batch_parser = subparsers.add_parser('batch', help='Analyze directory of photos')
    batch_parser.add_argument('directory', help='Directory containing photos')
    batch_parser.add_argument('--write', action='store_true', help='Write metadata to files')
    batch_parser.add_argument('--api-key', help='API key')
    
    # Search command
    search_parser = subparsers.add_parser('search', help='Search photos by metadata')
    search_parser.add_argument('directory', help='Directory to search')
    search_parser.add_argument('query', help='Search query')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Execute command
    if args.command == 'analyze':
        analyzer = PhotoAnalyzer(api_key=args.api_key)
        result = analyzer.analyze_photo(args.file)
        
        if result:
            print("\n📸 Analysis Results:")
            print(f"Title: {result['title']}")
            print(f"Description: {result['description']}")
            print(f"Tags: {', '.join(result['tags'])}")
            print(f"Subjects: {', '.join(result['subjects'])}")
            print(f"Scene: {result['scene_type']}")
            print(f"Confidence: {result['confidence']:.2f}")
            
            if args.write:
                if analyzer.write_metadata(args.file, result):
                    print("✅ Metadata written to file")
                else:
                    print("❌ Failed to write metadata")
        else:
            print("❌ Analysis failed")
            return 1
    
    elif args.command == 'batch':
        analyzer = PhotoAnalyzer(api_key=args.api_key)
        directory = Path(args.directory)
        
        # Find all images
        image_files = list(directory.rglob("*.jpg")) + \
                     list(directory.rglob("*.jpeg")) + \
                     list(directory.rglob("*.png"))
        
        print(f"\n📁 Found {len(image_files)} images")
        
        for i, filepath in enumerate(image_files, 1):
            print(f"\n[{i}/{len(image_files)}] Analyzing {filepath.name}...")
            
            result = analyzer.analyze_photo(str(filepath))
            if result:
                print(f"  ✓ {result['title']}")
                
                if args.write:
                    analyzer.write_metadata(str(filepath), result)
            else:
                print(f"  ✗ Failed")
        
        print("\n✅ Batch analysis complete")
    
    elif args.command == 'search':
        searcher = SimpleSearch(args.directory)
        results = searcher.search(args.query)
        
        print(f"\n🔍 Found {len(results)} matches for '{args.query}':")
        for filepath in results:
            print(f"  - {filepath}")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
