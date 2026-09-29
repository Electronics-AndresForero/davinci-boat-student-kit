#!/usr/bin/env python3
"""
Analiza un log capturado del ESP32 (ver borrador_tiempo.txt / borrador_mpu6500.txt)
y genera automaticamente los numeros que hay que pegar en el firmware.

USO:
  1. Corre el barco en modo RC (control remoto) con el firmware ya cargado --
     no importa si es borrador_tiempo.txt o borrador_mpu6500.txt, ambos loguean.
  2. Al terminar, saca el barco del agua, conecta el USB, abre un monitor serial
     (`pio device monitor` o el de Arduino IDE) y GUARDA toda la salida a un
     archivo de texto (o usa este mismo script en modo --puerto para capturarlo
     directo, ver abajo).
  3. Corre:  python3 analizar_log.py captura.txt
     o bien: python3 analizar_log.py --puerto /dev/ttyUSB0   (lee y reinicia solo)

QUE HACE:
  - Extrae las filas entre ===LOG_START=== y ===LOG_END=== (ignora todo lo demas
    que haya impreso el firmware antes/despues).
  - Si el log trae columnas de giroscopio (gyro_z_dps, heading_deg -- version
    MPU), reconstruye el rumbo tal cual lo integro el firmware.
  - Reconstruye una posicion (x, y) APROXIMADA asumiendo velocidad proporcional
    al throttle (constante V_MAX_MPS, ajustala abajo si tienes una medida real
    de velocidad -- si no, la forma del recorrido es confiable, la escala no).
  - Detecta automaticamente los tramos rectos vs. giro (mirando la velocidad de
    cambio de rumbo) e imprime un AUTO_SEQUENCE[] listo para pegar, mas
    (si hay giroscopio) el TURN_TARGET_DEG y los *_MS_CAP sugeridos.
  - Grafica: una vista 2D (x,y) de arriba, y una 3D (x, y, tiempo) -- la "3D"
    literal que se pidio; el eje tiempo muestra en que tramos se demoro mas.

Requiere: numpy, matplotlib (pip install numpy matplotlib).
"""
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 (registra el proyector 3D)

V_MAX_MPS = 0.64        # velocidad periferica de referencia (de resumen_cuaderno_ingenieria) --
                        # AJUSTAR si tienes un tiempo/distancia real medido para un tramo recto.
TURN_RATE_THRESHOLD_DPS = 15.0   # por encima de esto se considera "girando"
MIN_SEGMENT_MS = 300              # fusiona segmentos mas cortos que esto (ruido)


def extraer_log(texto):
    """Saca las filas entre los marcadores. Devuelve (encabezados, filas)."""
    lineas = texto.splitlines()
    try:
        i0 = lineas.index("===LOG_START===") + 1
        i1 = lineas.index("===LOG_END===")
    except ValueError:
        raise SystemExit("No encontre ===LOG_START===/===LOG_END=== en el archivo -- "
                          "¿es una captura completa del monitor serial?")
    cuerpo = [l for l in lineas[i0:i1] if l.strip()]
    if not cuerpo:
        raise SystemExit("El log esta vacio entre los marcadores.")
    encabezados = cuerpo[0].split(",")
    filas = [l.split(",") for l in cuerpo[1:]]
    return encabezados, filas


def cargar(path):
    with open(path, "r", errors="ignore") as f:
        return extraer_log(f.read())


def a_arrays(encabezados, filas):
    idx = {h: i for i, h in enumerate(encabezados)}
    n = len(filas)
    t_ms = np.array([float(r[idx["t_ms"]]) for r in filas])
    modo = np.array([int(r[idx["modo"]]) for r in filas])
    throttle = np.array([float(r[idx["throttle"]]) for r in filas])
    steer = np.array([float(r[idx["steer"]]) for r in filas])
    tiene_giro = "heading_deg" in idx
    heading = np.array([float(r[idx["heading_deg"]]) for r in filas]) if tiene_giro else None
    gyro_z = np.array([float(r[idx["gyro_z_dps"]]) for r in filas]) if tiene_giro else None
    return dict(t_ms=t_ms, modo=modo, throttle=throttle, steer=steer,
                heading=heading, gyro_z=gyro_z, tiene_giro=tiene_giro)


def reconstruir_trayectoria(d):
    """Integracion simple (Euler). x,y en metros -- ver disclaimer arriba sobre V_MAX_MPS."""
    n = len(d["t_ms"])
    x = np.zeros(n); y = np.zeros(n)
    if d["tiene_giro"]:
        heading_rad = np.radians(d["heading"])
    else:
        heading_rad = np.zeros(n)   # sin giroscopio no hay rumbo -- asume recto (solo referencia)
    for i in range(1, n):
        dt = (d["t_ms"][i] - d["t_ms"][i - 1]) / 1000.0
        v = V_MAX_MPS * d["throttle"][i]
        x[i] = x[i - 1] + v * np.cos(heading_rad[i]) * dt
        y[i] = y[i - 1] + v * np.sin(heading_rad[i]) * dt
    return x, y


def detectar_segmentos(d):
    """Agrupa el log en tramos 'recto' / 'giro' segun la velocidad de giro."""
    n = len(d["t_ms"])
    if d["tiene_giro"]:
        girando = np.abs(d["gyro_z"]) > TURN_RATE_THRESHOLD_DPS
    else:
        # sin giroscopio: aproxima "girando" por steer distinto de cero -- mucho mas crudo.
        girando = np.abs(d["steer"]) > 0.15

    segmentos = []
    inicio = 0
    estado_actual = girando[0]
    for i in range(1, n):
        if girando[i] != estado_actual:
            segmentos.append((inicio, i - 1, estado_actual))
            inicio = i
            estado_actual = girando[i]
    segmentos.append((inicio, n - 1, estado_actual))

    # fusiona segmentos muy cortos (ruido) con el anterior
    fusionados = []
    for seg in segmentos:
        i0, i1, giro = seg
        dur = d["t_ms"][i1] - d["t_ms"][i0]
        if fusionados and dur < MIN_SEGMENT_MS:
            pi0, pi1, pgiro = fusionados[-1]
            fusionados[-1] = (pi0, i1, pgiro)
        else:
            fusionados.append(seg)
    return fusionados


def imprimir_resultados(d, segmentos):
    print("\n--- Tramos detectados ---")
    pasos_cpp = []
    turn_deg_medido = None
    for (i0, i1, giro) in segmentos:
        dur_ms = d["t_ms"][i1] - d["t_ms"][i0]
        thr = float(np.mean(d["throttle"][i0:i1 + 1]))
        st  = float(np.mean(d["steer"][i0:i1 + 1]))
        etiqueta = "GIRO " if giro else "recto"
        extra = ""
        if giro and d["tiene_giro"]:
            delta = d["heading"][i1] - d["heading"][i0]
            extra = f"  delta_rumbo={delta:+.1f} deg"
            turn_deg_medido = abs(delta)
        print(f"  [{etiqueta}] dur={dur_ms:6.0f} ms  throttle={thr:+.2f}  steer={st:+.2f}{extra}")
        pasos_cpp.append((dur_ms, thr, st))

    print("\n--- Pegar en AUTO_SEQUENCE (borrador_tiempo.txt) ---")
    print("const AutoStep AUTO_SEQUENCE[] = {")
    for dur_ms, thr, st in pasos_cpp:
        print(f"  {{ {dur_ms:.0f}, {thr:.2f}f, {st:.2f}f }},")
    print("};")

    if d["tiene_giro"]:
        print("\n--- Sugerido para borrador_mpu6500.txt ---")
        if turn_deg_medido:
            print(f"constexpr float TURN_TARGET_DEG = {turn_deg_medido:.1f}f;  "
                  f"// medido en esta corrida")
        for (i0, i1, giro), (dur_ms, _, _) in zip(segmentos, pasos_cpp):
            margen = dur_ms * 1.5   # +50% de margen sobre lo medido, como red de seguridad
            nombre = "TURN_MS_CAP" if giro else "STRAIGHT*_MS_CAP"
            print(f"// tramo medido {dur_ms:.0f} ms -> sugerido {nombre} ~= {margen:.0f} ms")


def graficar(d, x, y):
    fig = plt.figure(figsize=(11, 5))

    ax2d = fig.add_subplot(1, 2, 1)
    ax2d.plot(x, y, "-", lw=1.5)
    paso = max(1, len(x) // 25)
    ax2d.quiver(x[::paso], y[::paso],
                np.cos(np.radians(d["heading"][::paso])) if d["tiene_giro"] else np.ones_like(x[::paso]),
                np.sin(np.radians(d["heading"][::paso])) if d["tiene_giro"] else np.zeros_like(x[::paso]),
                angles="xy", scale=20, width=0.004, color="crimson")
    ax2d.set_title("Vista de pajaro (x, y) -- APROXIMADA, ver disclaimer de V_MAX_MPS")
    ax2d.set_xlabel("x (m)"); ax2d.set_ylabel("y (m)")
    ax2d.axis("equal"); ax2d.grid(True, alpha=0.3)

    ax3d = fig.add_subplot(1, 2, 2, projection="3d")
    t_s = (d["t_ms"] - d["t_ms"][0]) / 1000.0
    ax3d.plot(x, y, t_s, lw=1.5)
    ax3d.set_title("Ruta en 3D (x, y, tiempo)")
    ax3d.set_xlabel("x (m)"); ax3d.set_ylabel("y (m)"); ax3d.set_zlabel("t (s)")

    plt.tight_layout()
    plt.show()


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("archivo", nargs="?", help="captura de texto del monitor serial")
    ap.add_argument("--puerto", help="puerto serial a leer en vivo, ej. /dev/ttyUSB0 "
                                      "(alternativa a pasar un archivo)")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    if args.puerto:
        import serial  # pip install pyserial -- solo hace falta para este modo
        print(f"Leyendo {args.puerto} @ {args.baud}... reinicia el ESP32 ahora (boton RESET).")
        with serial.Serial(args.puerto, args.baud, timeout=2) as ser:
            texto = ""
            capturando = False
            while True:
                linea = ser.readline().decode(errors="ignore")
                if not linea:
                    continue
                texto += linea
                if "LOG_START" in linea:
                    capturando = True
                if "LOG_END" in linea and capturando:
                    break
        encabezados, filas = extraer_log(texto)
    elif args.archivo:
        encabezados, filas = cargar(args.archivo)
    else:
        ap.error("pasa un archivo de captura o --puerto")

    d = a_arrays(encabezados, filas)
    x, y = reconstruir_trayectoria(d)
    segmentos = detectar_segmentos(d)
    imprimir_resultados(d, segmentos)
    graficar(d, x, y)


if __name__ == "__main__":
    main()
