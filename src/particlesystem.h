// SPDX-FileCopyrightText: 2004 Matthias Braun <matze@braunis.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_PARTICLESYSTEM_H
#define SUPERTUX_PARTICLESYSTEM_H

#include <vector>
#include "texture.h"

/**
 * This is the base class for particle systems. It is responsible for storing a
 * set of particles with each having an x- and y-coordinate the number of the
 * layer where it should be drawn and a texture.
 * The coordinate system used here is a virtual one. It would be a bad idea to
 * populate whole levels with particles. So we're using a virtual rectangle
 * here that is tiled onto the level when drawing. This rectangle has the size
 * (virtual_width, virtual_height). We're using modulo on the particle
 * coordinates, so when a particle leaves left, it'll reenter at the right
 * side.
 *
 * Classes that implement a particle system should subclass from this class,
 * initialize particles in the constructor and move them in the simulate
 * function.
 */
class ParticleSystem
{
public:
    ParticleSystem();
    virtual ~ParticleSystem();
    
    void draw(float scrollx, float scrolly, int layer);

    virtual void simulate(float elapsed_time) = 0;

protected:
    class Particle
    {
    public:
        virtual ~Particle()
        { }

        float x, y;
        int layer;
        Surface* texture;
    };
    
    std::vector<Particle*> particles;
    float virtual_width, virtual_height;
};

class SnowParticleSystem : public ParticleSystem
{
public:
    SnowParticleSystem();
    virtual ~SnowParticleSystem();

    virtual void simulate(float elapsed_time);
    
private:
    class SnowParticle : public Particle
    {
    public:
        float speed;
    };
    
    Surface* snowimages[3];
};

class CloudParticleSystem : public ParticleSystem
{
public:
    CloudParticleSystem();
    virtual ~CloudParticleSystem();

    virtual void simulate(float elapsed_time);
    
private:
    class CloudParticle : public Particle
    {
    public:
        float speed;
    };
    
    Surface* cloudimage;
};

#endif

