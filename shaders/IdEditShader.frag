#version 450
layout(location = 0) in flat uint inId;
layout(location = 0) out uint outId;

void main() {
    outId = inId;
}