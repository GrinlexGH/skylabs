struct Particle
{
    float2 position;
    float2 velocity;
    float4 color;
};

struct ParameterUBO
{
    float deltaTime;
};

[[vk::binding(0)]]
ConstantBuffer<ParameterUBO> ubo;

[[vk::binding(1)]]
StructuredBuffer<Particle> particlesIn;

[[vk::binding(2)]]
RWStructuredBuffer<Particle> particlesOut;

[numthreads(256, 1, 1)]
[shader("compute")]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;

    Particle particleIn = particlesIn[index];

    particlesOut[index].position = particleIn.position + particleIn.velocity * ubo.deltaTime;
    particlesOut[index].velocity = particleIn.velocity;
    particlesOut[index].color = particleIn.color;

    if (particlesOut[index].position.x <= -1.0 || particlesOut[index].position.x >= 1.0)
    {
        particlesOut[index].velocity.x = -particlesOut[index].velocity.x;
    }
    if (particlesOut[index].position.y <= -1.0 || particlesOut[index].position.y >= 1.0)
    {
        particlesOut[index].velocity.y = -particlesOut[index].velocity.y;
    }
}
