#!/usr/bin/env python3
"""
Genera, desde los .mmd de esta carpeta y NADA MAS:
  - PDF vectorial        (mermaid-cli --pdfFit + pdfcrop)  <- lo que usa LaTeX
  - PNG y SVG            (para pegar en chat, slides o web)
  - la nota del Vault    (mermaid embebido, Obsidian lo renderiza nativo)
  - diagramas.pdf        guia de clase, A4
  - cartel.pdf           cartel A3 APAISADO para el stand

Fuente unica: si cambias un .mmd, corre esto y los tres salen al dia.
    python3 build.py
"""
import subprocess, pathlib, shutil, os

HERE  = pathlib.Path(__file__).parent.resolve()
MMDC  = pathlib.Path.home()/".local/share/mermaid-cli/node_modules/.bin/mmdc"
VAULT = pathlib.Path.home()/"Documents/Vault/01-Projects/electronics/davinci-paddle-boat"
# los dos primeros van a la nota del Vault; el tercero es solo para el cartel
DIAGS  = [("estados-e5", None), ("cadena-mando-agua", None)]
EXTRA  = [("cartel-sistema", None)]

def render():
    for name, _ in DIAGS + EXTRA:
        # PDF vectorial: --pdfFit para que no recorte contra una pagina fija,
        # y pdfcrop para quitar el margen sobrante.
        for ext, extra in ((".pdf", ["--pdfFit"]), (".png", ["-s","3"]), (".svg", [])):
            subprocess.run(["node", str(MMDC), "-i", f"{name}.mmd",
                            "-o", f"{name}{ext}", "-b", "white", *extra],
                           cwd=HERE, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["pdfcrop", "--margins", "4", f"{name}.pdf", f"{name}.pdf"],
                       cwd=HERE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"  {name}: pdf (vectorial) + png + svg")

def vault():
    body = (HERE/"_nota.md").read_text()
    for name, _ in DIAGS:
        src = (HERE/f"{name}.mmd").read_text().strip()
        # el front-matter ---title--- de mermaid no lo entiende Obsidian: fuera
        if src.startswith("---"):
            src = src.split("---", 2)[2].strip()
        body = body.replace(f"{{{{{name}}}}}", "```mermaid\n" + src + "\n```")
    out = VAULT/"Diagramas del sistema.md"
    out.write_text(body)
    print(f"  {out}")

def pdf():
    for doc in ("diagramas", "cartel"):
        for _ in range(2):
            subprocess.run(["pdflatex","-interaction=nonstopmode",f"{doc}.tex"],
                           cwd=HERE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for ext in (".aux",".log",".out"):
            (HERE/f"{doc}{ext}").unlink(missing_ok=True)
        print(f"  {doc}.pdf")
    (HERE/"texput.log").unlink(missing_ok=True)

if __name__ == "__main__":
    print("renderizando..."); render()
    print("nota del Vault..."); vault()
    print("PDF...");            pdf()
    print("listo.")
