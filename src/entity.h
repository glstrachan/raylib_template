#pragma once

#include "raylib.h"
#include "../external/core.h"

Enum(EntityType, uint32,
    ENTITY_INVALID = 0,
    ENTITY_PLAYER,
    ENTITY_DOOR,
);

typedef int EntityId;

typedef struct {
    EntityType type;
    Vector4 position;

    union {
        struct {
            string name;
            EntityId target;
        } player;

        struct {
            bool opened;
        } door;
    };
} Entity;

#define ENTITIES_MAX_COUNT 1024u
extern Entity entities[ENTITIES_MAX_COUNT];
extern EntityId next_entity_id;

// void entity_update()
