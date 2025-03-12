#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define PARTICLES 1000000
#define ITERATIONS 200

// Traditional Array of Structs (AoS)
struct ParticleAoS {
    float x, y, z;       // Position
    float vx, vy, vz;    // Velocity
    float mass;          // Mass
    float density;       // Additional property to make the struct less optimal
};

// Struct of Arrays (SoA) equivalent
struct ParticleSystemSoA {
    float *x, *y, *z;    // Position arrays
    float *vx, *vy, *vz; // Velocity arrays
    float *mass;         // Mass array
    float *density;      // Additional property
};

// Volatile to prevent compiler optimization
volatile float sink_float;

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main() {
    double start, end;
    double elapsed_aos, elapsed_soa;

    printf("Struct size - AoS: %zu bytes\n", sizeof(struct ParticleAoS));

    // Allocate memory
    struct ParticleAoS *particles_aos = malloc(PARTICLES * sizeof(struct ParticleAoS));

    struct ParticleSystemSoA particles_soa;
    particles_soa.x = malloc(PARTICLES * sizeof(float));
    particles_soa.y = malloc(PARTICLES * sizeof(float));
    particles_soa.z = malloc(PARTICLES * sizeof(float));
    particles_soa.vx = malloc(PARTICLES * sizeof(float));
    particles_soa.vy = malloc(PARTICLES * sizeof(float));
    particles_soa.vz = malloc(PARTICLES * sizeof(float));
    particles_soa.mass = malloc(PARTICLES * sizeof(float));
    particles_soa.density = malloc(PARTICLES * sizeof(float));

    if (!particles_aos || !particles_soa.x) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Initialize with same values
    for (int i = 0; i < PARTICLES; i++) {
        // AoS initialization
        particles_aos[i].x = (float)(i % 1000) / 100.0f;
        particles_aos[i].y = (float)((i + 500) % 1000) / 100.0f;
        particles_aos[i].z = (float)((i + 1000) % 1000) / 100.0f;
        particles_aos[i].vx = 0.1f;
        particles_aos[i].vy = 0.2f;
        particles_aos[i].vz = 0.3f;
        particles_aos[i].mass = 1.0f + (i % 10) / 10.0f;
        particles_aos[i].density = 2.0f;

        // SoA initialization with identical values
        particles_soa.x[i] = particles_aos[i].x;
        particles_soa.y[i] = particles_aos[i].y;
        particles_soa.z[i] = particles_aos[i].z;
        particles_soa.vx[i] = particles_aos[i].vx;
        particles_soa.vy[i] = particles_aos[i].vy;
        particles_soa.vz[i] = particles_aos[i].vz;
        particles_soa.mass[i] = particles_aos[i].mass;
        particles_soa.density[i] = particles_aos[i].density;
    }

    // Warmup
    float warmup = 0;
    for (int i = 0; i < PARTICLES / 10; i++) {
        warmup += particles_aos[i].x + particles_soa.x[i];
    }
    sink_float = warmup;

    // Benchmark AoS - more complex physics update
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < PARTICLES; i++) {
            // Position update
            particles_aos[i].x += particles_aos[i].vx * 0.1f;
            particles_aos[i].y += particles_aos[i].vy * 0.1f;
            particles_aos[i].z += particles_aos[i].vz * 0.1f;

            // Apply "gravity" - using mass
            particles_aos[i].vy -= 0.01f * particles_aos[i].mass;

            // Apply "drag" - proportional to velocity squared
            float v_squared = particles_aos[i].vx * particles_aos[i].vx +
            particles_aos[i].vy * particles_aos[i].vy +
            particles_aos[i].vz * particles_aos[i].vz;
            float drag_factor = 0.001f * particles_aos[i].density * v_squared;

            // Update velocities
            particles_aos[i].vx *= (1.0f - drag_factor);
            particles_aos[i].vy *= (1.0f - drag_factor);
            particles_aos[i].vz *= (1.0f - drag_factor);
        }
    }
    end = get_time_ms();
    elapsed_aos = end - start;

    // Make sure some values are seen
    sink_float = particles_aos[PARTICLES/2].x + particles_aos[PARTICLES/2].vy;

    // Benchmark SoA - same physics update
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < PARTICLES; i++) {
            // Position update
            particles_soa.x[i] += particles_soa.vx[i] * 0.1f;
            particles_soa.y[i] += particles_soa.vy[i] * 0.1f;
            particles_soa.z[i] += particles_soa.vz[i] * 0.1f;

            // Apply "gravity" - using mass
            particles_soa.vy[i] -= 0.01f * particles_soa.mass[i];

            // Apply "drag" - proportional to velocity squared
            float v_squared = particles_soa.vx[i] * particles_soa.vx[i] +
            particles_soa.vy[i] * particles_soa.vy[i] +
            particles_soa.vz[i] * particles_soa.vz[i];
            float drag_factor = 0.001f * particles_soa.density[i] * v_squared;

            // Update velocities
            particles_soa.vx[i] *= (1.0f - drag_factor);
            particles_soa.vy[i] *= (1.0f - drag_factor);
            particles_soa.vz[i] *= (1.0f - drag_factor);
        }
    }
    end = get_time_ms();
    elapsed_soa = end - start;

    // Make sure some values are seen
    sink_float = particles_soa.x[PARTICLES/2] + particles_soa.vy[PARTICLES/2];

    printf("Array of Structs time: %.3f ms\n", elapsed_aos);
    printf("Struct of Arrays time: %.3f ms\n", elapsed_soa);
    printf("Performance improvement: %.2f%%\n",
           (elapsed_aos - elapsed_soa) / elapsed_aos * 100);

    // Cleanup
    free(particles_aos);
    free(particles_soa.x);
    free(particles_soa.y);
    free(particles_soa.z);
    free(particles_soa.vx);
    free(particles_soa.vy);
    free(particles_soa.vz);
    free(particles_soa.mass);
    free(particles_soa.density);

    return 0;
}
