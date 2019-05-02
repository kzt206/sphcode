#include <algorithm>

#include "defines.hpp"
#include "fluid_force.hpp"
#include "particle.hpp"
#include "distance.hpp"
#include "simulation.hpp"
#include "kernel/kernel_function.hpp"

namespace sph
{

void FluidForce::initialize(std::shared_ptr<SPHParameters> param)
{
    m_neighbor_number = param->physics.neighbor_number;
}

void FluidForce::calculation(std::shared_ptr<Simulation> sim)
{
    auto & particles = sim->get_particles();
    auto * distance = sim->get_distance().get();
    const int num = sim->get_particle_num();
    auto * kernel = sim->get_kernel().get();

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        auto & p_i = particles[i];
        std::vector<int> neighbor_list(m_neighbor_number * neighbor_list_size);
        
        // neighbor search
        int const n_neighbor = exhaustive_search(p_i, p_i.sml, particles, num, neighbor_list, m_neighbor_number * neighbor_list_size, distance);

        // fluid force
        const vec_t & r_i = p_i.pos;
        const vec_t & v_i = p_i.vel;
        const real p_per_rho2_i = p_i.pres / sqr(p_i.dens);
        const real h_i = p_i.sml;

        vec_t acc(0.0);
        real dene = 0.0;

        for(int n = 0; n < n_neighbor; ++n) {
            int const j = neighbor_list[n];
            auto & p_j = particles[j];
            const vec_t r_ij = distance->calc_r_ij(r_i, p_j.pos);
            const real r = abs(r_ij);

            if(r >= std::max(h_i, p_j.sml) || r == 0.0) {
                continue;
            }

            const vec_t dw_i = kernel->dw(r_ij, r, h_i);
            const vec_t dw_j = kernel->dw(r_ij, r, p_j.sml);
            const vec_t dw_ij = (dw_i + dw_j) * 0.5;

            const real pi_ij = artificial_viscosity(p_i, p_j);

            acc -= dw_ij * (p_j.mass * (p_per_rho2_i + p_j.pres / sqr(p_j.dens) + pi_ij));
            dene += p_j.mass * (p_per_rho2_i + 0.5 * pi_ij) * inner_product(v_i - p_j.vel, dw_ij);
        }

        p_i.acc = acc;
        p_i.dene = dene;
    }
}

real FluidForce::artificial_viscosity(const SPHParticle & p_i, const SPHParticle & p_j)
{
    // Monaghan (1997)
    const auto v_ij = p_i.vel - p_j.vel;
    const auto r_ij = p_i.pos - p_j.pos;
    const real vr = inner_product(v_ij, r_ij);

    if(vr < 0) {
        const real alpha = 0.5 * (p_i.alpha + p_j.alpha);
        const real balsara = 0.5 * (p_i.balsara + p_j.balsara);
        const real w_ij = vr / abs(r_ij);
        const real v_sig = p_i.sound + p_j.sound - 3.0 * w_ij;
        const real rho_ij_inv = 2.0 / (p_i.dens + p_j.dens);
        
        const real pi_ij = -0.5 * balsara * alpha * v_sig * w_ij * rho_ij_inv;
        return pi_ij;
    } else {
        return 0;
    }
}

int FluidForce::exhaustive_search(
    SPHParticle & p_i,
    const real kernel_size,
    const std::vector<SPHParticle> & particles,
    const int num,
    std::vector<int> & neighbor_list,
    const int list_size,
    Distance const * distance)
{
    const real kernel_size_i2 = kernel_size * kernel_size;
    const vec_t & pos_i = p_i.pos;
    int count = 0;
    for(int j = 0; j < num; ++j) {
        const auto & p_j = particles[j];
        const vec_t r_ij = distance->calc_r_ij(pos_i, p_j.pos);
        const real r2 = abs2(r_ij);
        const real kernel_size2 = std::max(kernel_size_i2, p_j.sml * p_j.sml);
        if(r2 < kernel_size2) {
            neighbor_list[count] = j;
            ++count;
        }
    }

    std::sort(neighbor_list.begin(), neighbor_list.begin() + count, [&](const int a, const int b) {
        const vec_t r_ia = distance->calc_r_ij(pos_i, particles[a].pos);
        const vec_t r_ib = distance->calc_r_ij(pos_i, particles[b].pos);
        return abs2(r_ia) < abs2(r_ib);
    });

    return count;
}

}