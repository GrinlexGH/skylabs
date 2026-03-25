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

    Particle particle = particlesIn[index];

    particle.position += particle.velocity * pc.deltaTime * 100.0f;

    if (particle.position.x <= -1.0 || particle.position.x >= 1.0) {
        particle.velocity.x = -particle.velocity.x;
    }
    
    if (particle.position.y <= -1.0 || particle.position.y >= 1.0) {
        particle.velocity.y = -particle.velocity.y;
    }

    particlesOut[index] = particle;
}