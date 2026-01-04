# PhotoGuru Python Agents

This directory contains the Python-based AI analysis agents for PhotoGuru.

## Files

- **agent_mvp.py** - Simplified MVP agent using cloud-based OpenAI GPT-4 Vision
- **agent_v2.py** - Advanced agent with CLIP embeddings and SKP (Semantic Key Protocol)
- **requirements_mvp.txt** - Dependencies for MVP agent

## Installation

```bash
# For MVP agent (cloud-based)
pip install -r requirements_mvp.txt

# For advanced agent (local CLIP)
pip install -r ../requirements.txt
```

## Usage

### MVP Agent (Recommended)

```bash
# Set your OpenAI API key
export OPENAI_API_KEY="sk-..."

# Analyze a single photo
python agent_mvp.py analyze photo.jpg --write

# Analyze a directory
python agent_mvp.py analyze /path/to/photos --write
```

### Advanced Agent (v2)

```bash
# Generate embeddings for photos
python agent_v2.py write /path/to/photos

# Search using natural language
python agent_v2.py search "sunset at the beach"
```

## Integration with C++ Application

The C++ application automatically detects these agents and uses them for AI-powered features. The agents are called via subprocess when needed.
