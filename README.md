# opengl-black-hole-sim
The evolution of a black hole simulation from a CPU-bound C/OpenGL implementation to a high-performance GPU-accelerated version


# The Journey of a Black Hole Simulation

This repository documents the creation of a real-time black hole simulation, showcasing its evolution from a standard CPU-bound application to a high-performance, GPU-accelerated version.

![GIF of the final GPU simulation]

## Project Overview
The goal was to create a scientifically-inspired visualization of gravitational lensing around a black hole using C and OpenGL. The project is split into two main versions, each in its own directory, demonstrating a significant leap in performance and programming paradigm.

---
## Scientific & Mathematical Approach

This simulation is not a precise GR solver but a compelling visual approximation based on the core principles of General Relativity.

### From Spacetime Curvature to Gravity
The fundamental concept of General Relativity is that mass does not create a "force" of gravity. Instead, **mass tells spacetime how to curve, and curved spacetime tells matter how to move**. Our simulation visualizes this with the classic "bowling ball on a rubber sheet" analogy. The 3D "gravity well" represents the warped spacetime around the black hole.

A photon traveling through this region will always follow the straightest possible path, known as a **geodesic**. On our curved grid, this straightest path appears to us as a bent or curved trajectory. This is the essence of gravitational lensing.

### The Physics Model
To simulate the photon's path along this geodesic without solving the full Einstein Field Equations, we use a computational shortcut. We calculate the "downhill pull" on our spacetime grid at the photon's location. This is done by finding the **negative gradient** of the gravitational potential.

1.  We approximate the gravitational potential with the formula:
    $$U(r) \propto -\frac{M}{r}$$
    where $M$ is the mass of the black hole and $r$ is the distance from the center.

2.  The force is the negative gradient of this potential, $\vec{F} = -\nabla U$. This results in a force vector that points towards the center with a magnitude that follows the inverse-square law, which we implemented in the GPU compute shader:
    $$\vec{F} \propto -\frac{M\vec{r}}{r^3}$$

3.  In each frame, this force vector is used to incrementally update the photon's velocity, guiding it along the curved spacetime.

### Simulating Black Hole Phenomena
This model allows us to simulate the three possible fates of a photon:

* **Escape (Gravitational Lensing):** Photons that pass far from the black hole have their paths slightly bent by the curvature but possess enough velocity to continue on their journey. This is the classic weak lensing effect.

* **Capture (Event Horizon):** We define a `CAPTURE_RADIUS` which represents the event horizon. If a photon's trajectory brings it within this distance of the center, it is considered captured and is removed from the simulation, as it would be unable to escape in reality.

* **Orbit (Photon Sphere):** To create the visually distinct photon sphere, we implemented a "trapping zone" — a narrow ring around the black hole. When a photon enters this zone, its radial velocity (its speed directly towards or away from the center) is artificially dampened. This nudges the photon into a more stable circular path, causing it to "spiral" and get stuck in orbit, dynamically forming the glowing ring of light from the incoming stream.

---
## 1. GPU-Accelerated Version (Final)
(...The rest of the README follows, no changes needed...)