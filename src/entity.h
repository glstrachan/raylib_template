#pragma once

#include "raylib.h"
#include "../external/core.h"

Enum(EntityType, uint32,
    ENTITY_INVALID = 0,
    ENTITY_PLAYER,
    ENTITY_DOOR,
    ENTITY_INTERACT,
    ENTITY_PLATFORM,
);

typedef int EntityId;

typedef struct {
    EntityType type;
    Vector2 position;

    union {
        struct {
            Vector2 speed;
            EntityId interacting_with;
        } player;

        // struct {
        //     EntityId
        // } interactable;

        struct {
            Vector2 size;
        } platform;

        struct {
            bool opened;
        } door;
    };
} Entity;

#define ENTITIES_MAX_COUNT 1024u
extern Entity entities[ENTITIES_MAX_COUNT];
extern EntityId next_entity_id;

// void entity_update()
