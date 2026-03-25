struct Particle {
    float2 position;
    float2 velocity;
    float4 color;
};

struct PushConstants {
    float deltaTime;
};

[[vk::push_constant]]
PushConstants pc;

[[vk::binding(0, 0)]]
StructuredBuffer<Particle> particlesIn;

[[vk::binding(1, 0)]]
RWStructuredBuffer<Particle> particlesOut;

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;

    Particle particleIn = particlesIn[index];

    particlesOut[index].position = particleIn.position + particleIn.velocity.xy * pc.deltaTime;
    particlesOut[index].velocity = particleIn.velocity;

    if ((particlesOut[index].position.x <= -1.0) || (particlesOut[index].position.x >= 1.0)) {
        particlesOut[index].velocity.x = -particlesOut[index].velocity.x;
    }
    if ((particlesOut[index].position.y <= -1.0) || (particlesOut[index].position.y >= 1.0)) {
        particlesOut[index].velocity.y = -particlesOut[index].velocity.y;
    }
}
