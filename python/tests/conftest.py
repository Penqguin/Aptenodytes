import sys
from pathlib import Path

# Ensure python/ is on path for `import backtester`
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
