#version 450

#define NUM_PARTICLES (1024*1024) // total number of particles to move
#define NUM_WORK_ITEMS_PER_GROUP 64 // # work-items per work-group
#define NUM_X_WORK_GROUPS ( NUM_PARTICLES / NUM_WORK_ITEMS_PER_GROUP )
struct pos {
    glm::vec4; // positions
};
struct vel {
    glm::vec4; // velocities
};
struct col {
    glm::vec4; // colors
};