# Diagramas del sistema

> [!info] De dónde salen
> Generados desde `davinci-boat-student-kit/diagramas/*.mmd` con `python3 build.py`.
> **No los edites aquí** — edita el `.mmd` y vuelve a correr el script, que también
> rehace el PDF. Los mismos diagramas están en `diagramas/diagramas.pdf` para imprimir.

Estos dos diagramas son lo único del proyecto que **merece** un diagrama. El código de
cada ejercicio son 60–99 líneas y cabe en una pantalla: se lee, no se dibuja. Lo que no
cabe en la cabeza es (1) en qué estado está el barco y (2) la cadena completa desde el
mando hasta el agua, que atraviesa firmware, electrónica y mecánica.

---

## 1. Los estados del barco

Es lo más difícil de [[Phase 5 — Modes, Arming and Failsafe]], y es un concepto de
**seguridad**: mientras el barco no esté *armado*, el motor no gira pase lo que pase.

{{estados-e5}}

Hay **dos cosas independientes** que la gente confunde:

| | Quién lo decide | Qué significa |
|---|---|---|
| **Modo** | el interruptor MODE (GPIO 26) | de dónde vienen las órdenes: del mando (RC) o del planificador (AUTO) |
| **Armado** | el interruptor ARM (GPIO 32) **y** el failsafe | si se le hace caso a esas órdenes o se manda 0 |

Se puede estar en AUTO y desarmado. Se puede estar en RC y desarmado. **El modo no arma
nada.**

### El failsafe tiene dos gatillos

`applyFailsafe()` fuerza `armed = false` si **cualquiera** de los dos se cumple:

1. El interruptor ARM está abierto (lee HIGH).
2. La orden más reciente tiene más de `FAILSAFE_MS` = **500 ms**.

El segundo es el que salva el barco: si el mando se apaga o se sale de rango, no se queda
navegando con el último throttle recibido — para solo.

### Detalle que se pasa por alto

En el flanco de cambio de modo, `applied` se pone a **0**. Es decir: al cambiar de RC a
AUTO o al revés, la rampa de aceleración **empieza de cero**, no desde donde iba. Sin eso,
al saltar a AUTO el barco arrancaría de golpe con la velocidad que llevaba.

### El LED dice en qué estado estás

| LED | Estado |
|---|---|
| Fijo | armado — el motor puede girar |
| Parpadeo lento (400 ms) | RC, desarmado |
| Parpadeo rápido (150 ms) | AUTO, desarmado o run terminado |

---

## 2. Del mando al agua

Éste es **el** diagrama del proyecto, porque es el único sitio donde el firmware y todo el
trabajo mecánico aparecen juntos. Los colores marcan el dominio: azul lo que se programa,
rojo lo que se conecta, marrón lo que se corta y se imprime.

{{cadena-mando-agua}}

Cosas que se ven mejor aquí que leyendo el código:

- **Una sola estructura `Command` alimenta las dos ramas.** Propulsión y gobierno no son
  dos programas: son dos consumidores del mismo dato.
- **El failsafe está antes de la bifurcación**, así que apaga las dos cosas a la vez. No
  hay forma de quedarse con timón pero sin motor, ni al revés.
- **Cada flecha de la mitad de abajo es una pieza física** que hay que cortar, imprimir o
  pegar. La cadena mecánica es más larga que la de software.
- **Dónde se pierde y se gana.** El tren 17:25 baja de 250 a 170 rpm, y a cambio multiplica
  el par por 1.47. El cuadrilátero, en cambio, es **1:1 a propósito** — ver
  [[Phase 3 — The Servo Rudder]] y `demonstrative/guia-timon.pdf`.

---

## Cómo usarlos en clase

1. **Antes de escribir código:** el diagrama 2, tapando la mitad de abajo. «¿Qué tiene que
   producir el firmware?» → un `Command`. Eso es todo.
2. **En [[Phase 5 — Modes, Arming and Failsafe]]:** el diagrama 1, y que dibujen ellos las
   transiciones antes de verlo. Casi siempre se les olvida que el failsafe tiene dos
   gatillos, no uno.
3. **Al final:** el diagrama 2 entero, para que vean que el `throttle` que escribieron
   acaba siendo empuje en el agua a través de once cosas.
