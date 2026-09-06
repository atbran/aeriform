#pragma once
#include <array>
#include <complex>
#include <cmath>
namespace aeriform::dsp {
/** In-place radix-two FFT. Tables are initialized before processing; transforms
    have no heap allocation, locks, external plans or shared mutable state. */
template<int Order> class FixedFFT {
public:
    static constexpr int size=1<<Order;
    using Complex=std::complex<float>;
    void prepare() noexcept {
        for(int i=0;i<size;++i){unsigned x=(unsigned)i,y=0;for(int b=0;b<Order;++b){y=(y<<1)|(x&1);x>>=1;}reverse[(size_t)i]=(int)y;}
        for(int i=0;i<size/2;++i){float phase=-6.283185307179586f*i/size;twiddle[(size_t)i]={std::cos(phase),std::sin(phase)};}
    }
    void transform(std::array<Complex,size>& data,bool inverse=false) const noexcept {
        for(int i=0;i<size;++i)if(i<reverse[(size_t)i])std::swap(data[(size_t)i],data[(size_t)reverse[(size_t)i]]);
        for(int length=2;length<=size;length*=2){const int half=length/2,stride=size/length;
            for(int start=0;start<size;start+=length)for(int j=0;j<half;++j){auto w=twiddle[(size_t)(j*stride)];if(inverse)w=std::conj(w);auto even=data[(size_t)(start+j)],odd=w*data[(size_t)(start+j+half)];data[(size_t)(start+j)]=even+odd;data[(size_t)(start+j+half)]=even-odd;}}
        if(inverse)for(auto& value:data)value/=size;
    }
private:
    std::array<int,size> reverse{};
    std::array<Complex,size/2> twiddle{};
};
}
