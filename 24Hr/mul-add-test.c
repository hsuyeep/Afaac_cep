#include <stdio.h>
#include <xmmintrin.h>


__m128 v;


int main()
{
#pragma omp parallel
  {
    __m128 a = v, b = v, c = v, d = v, e = v, f = v, g = v, h = v;

    for (long long i = 1000000000; -- i >= 0;) {
      a *= a;
      b *= b;
      c *= c;
      d *= d;
      e += e;
      f += f;
      g += g;
      h += h;
    }

    v = a + b + c + d + e + f + g + h;
  }

  return 0;
}
