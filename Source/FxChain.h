#pragma once
#include <cstddef>
#include <array>
#include <memory>
namespace kyoto {
constexpr int kNumEffects = 200;
constexpr int kMaxFxSlots = 128;
struct FxSlotParams { int effect=-1; bool on=true; float amount=0.5f,tone=0.5f,motion=0.5f,mix=0.5f,shape=0.5f; };
class FxChain { public: FxChain(); ~FxChain(); void prepare(double); void reset(); void process(float*,float*,int,const FxSlotParams*,int); static int categoryOf(int); static const char* categoryName(int); static constexpr int numCategories=8; private: struct Impl; std::unique_ptr<Impl> impl; };
}
