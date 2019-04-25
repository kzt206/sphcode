#include <iostream>
#include <fstream>

#include "wendland_kernel.hpp"
#include "cubic_spline.hpp"

int main()
{
    std::ios_base::sync_with_stdio(false);

    std::cout << "compare dw(x) to (w(x + dx/2) - w(x - dx/2)) / dx" << std::endl;

    const real dx = 1.0 / 100;

    // spline
    {
        std::cout << "cubic spline" << std::endl;
        sph::Spline::Cubic cs;
        std::ofstream out_s("spline.dat");
        out_s << "\"x\", \"dw(x)\", \"(w(x + dx/2) - w(x - dx/2)) / dx\"" << std::endl;

        for(real x = dx * 0.5; x < 1.0; x += dx) {
            const vec_t r(x);
            auto dw1 = cs.dw(r, x, 1.0);
            auto dw2 = (cs.w(x + dx * 0.5, 1.0) - cs.w(x - dx * 0.5, 1.0)) / dx;
            out_s << x << ", " << dw1[0] << ", " << dw2 << std::endl;
        }
    }

    // wendland
    {
        std::cout << "Wendland C4" << std::endl;
        sph::Wendland::C4Kernel wl;
        std::ofstream out_w("wendland.dat");
        out_w << "\"x\", \"dw(x)\", \"(w(x + dx/2) - w(x - dx/2)) / dx\"" << std::endl;

        for(real x = dx * 0.5; x < 1.0; x += dx) {
            const vec_t r(x);
            auto dw1 = wl.dw(r, x, 1.0);
            auto dw2 = (wl.w(x + dx * 0.5, 1.0) - wl.w(x - dx * 0.5, 1.0)) / dx;
            out_w << x << ", " << dw1[0] << ", " << dw2 << std::endl;
        }
    }

    std::cout << "finish!" << std::endl;

    return 0;
}
