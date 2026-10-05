"""Count boundary and nonmanifold edges in the supplied ASCII reconstructions."""
from pathlib import Path
from collections import Counter

for path in sorted(Path("Assets/Models/happy_recon").glob("*.ply")):
    with path.open() as source:
        vertices = faces = 0
        for line in source:
            fields = line.split()
            if fields[:2] == ["element", "vertex"]:
                vertices = int(fields[2])
            if fields[:2] == ["element", "face"]:
                faces = int(fields[2])
            if line.strip() == "end_header":
                break
        for _ in range(vertices):
            next(source)
        edges = Counter()
        for _ in range(faces):
            indices = list(map(int, next(source).split()))[1:]
            for a, b in zip(indices, indices[1:] + indices[:1]):
                edges[min(a, b), max(a, b)] += 1
        print(path.name, "faces:", faces, "boundary edges:",
              sum(n == 1 for n in edges.values()), "nonmanifold edges:",
              sum(n > 2 for n in edges.values()))
