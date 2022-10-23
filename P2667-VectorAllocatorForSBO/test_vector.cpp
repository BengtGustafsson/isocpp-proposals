

#include "experimental_vector.h"

#include <cassert>

int main()
{
    std::vector<int> x;

    x.push_back(1);
    assert(x.size() == 1);

    std::static_vector<int, 3> s;

    s.push_back(1);
    assert(s.size() == 1);

    std::sbo_vector<int, 3> sbo;

    sbo.push_back(1);
    assert(sbo.size() == 1);
    sbo.push_back(2);
    assert(sbo.size() == 2);
    sbo.push_back(3);
    assert(sbo.size() == 3);
    sbo.push_back(4);
    assert(sbo.size() == 4);
    sbo.push_back(5);
    assert(sbo.size() == 5);

    assert(sbo[0] == 1);
    assert(sbo[1] == 2);
    assert(sbo[2] == 3);
    assert(sbo[3] == 4);
    assert(sbo[4] == 5);

    std::vector<int> v(std::move(sbo));
    sbo = std::move(v);
    std::static_vector<int, 10> svec(std::move(sbo));
    assert(svec[0] == 1);
    assert(svec[1] == 2);
    assert(svec[2] == 3);
    assert(svec[3] == 4);
    assert(svec[4] == 5);

    v = std::move(svec);
    assert(v[0] == 1);
    assert(v[1] == 2);
    assert(v[2] == 3);
    assert(v[3] == 4);
    assert(v[4] == 5);
}