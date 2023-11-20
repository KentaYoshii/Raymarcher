# PhongTotal

* [Basic Information](#basic-information)
    * [Members](#members)
* [Project Proposal](#project-proposal)
    * [Idea](#idea)
    * [Technical Features](#technical-features)
    * [Implementation Steps](#implementation-steps)
* [References](#references)

## Basic Information
### __Members__
| Name | cslogin | Github name |
|----|----|----|
| Kenta Yoshii | kyoshii | KentaYoshii |
| Alex Bao | abao5 | abao929 | 
| Brian Sutioso | bsutioso | BrianEdmund |

## Project Proposal
### __Idea__
Using Raymarching, generating cool scenes with fractals, terrains and other non-implicit objects (clouds, water, etc.) that are produced using procedural generation

### __Technical Features__
- Sign Distance Fields Raymarcher
    - A technique that can be used to generate fully procedural environments entirely from a single fragment shader.
- Fractal Generation
    - Mandlebulb Fractals with user-modifiable parameters (depth, height, etc.)
- Non-implicit Objects Generation
    - Terrain
        - Gradient Noise 
        - Value Noise
    - Water 
    - Clouds
    - Other Fractals (trees, plants, etc.)
- Lighting and Coloring
    - shadows, reflections, refractions
    - ssao

### __Implementation Steps__
1. Build a CPU Raymarcher
    - Test with simple primitives such as Cube and Cylinder
2. Generate Mandlebulb Fractals
3. Procedually generate Terrain
4. Procedually generate other non-implicit objects
5. (Bonus) Make it realtime
6. (Bonus) Produce other Fractals

## References
- Raymarching
    - Basics
        - [1](https://iquilezles.org/articles/terrainmarching/
)
        - [2](https://michaelwalczyk.com/blog-ray-marching.html)
        - [3](https://iquilezles.org/articles/nvscene2008/rwwtt.pdf)
        - [4](https://www.youtube.com/watch?v=Cp5WWtMoeKg)
    - List of SDFs
        - [5](https://iquilezles.org/articles/distfunctions/)
- Fractal Generations
    - [6](http://blog.hvidtfeldts.net/index.php/2011/06/distance-estimated-3d-fractals-part-i/)
    - [7](https://iquilezles.org/articles/mandelbulb/)
- Terrain Generations
    - [8](https://iquilezles.org/articles/morenoise/)
- Cloud Generation
    - [9](https://iquilezles.org/articles/dynclouds/)
- Water Terrain Generation
    - [10](https://iquilezles.org/articles/simplewater/)