#ifndef GENERAL_COMPONENTS_H
#define GENERAL_COMPONENTS_H

#include <stdint.h>

#include "System/ECPS/entityComponentProcessSystem.h"
#include "Math/vector2.h"
#include "Graphics/color.h"
#include "tween.h"

// general use components that are shared between games
typedef struct {
	Vector2 currPos;
	Vector2 futurePos;
} GCPosData;
extern ComponentID gcPosCompID;

typedef struct {
	Color currClr;
	Color futureClr;
} GCColorData;
extern ComponentID gcClrCompID;

typedef struct {
	float currRot;
	float futureRot;
} GCRotData;
extern ComponentID gcRotCompID;

typedef struct {
	Vector2 currScale;
	Vector2 futureScale;
} GCScaleData;
extern ComponentID gcScaleCompID;

typedef struct {
	float currValue;
	float futureValue;
} GCFloatVal0Data;
extern ComponentID gcFloatVal0CompID;

typedef struct {
	int img;
	int8_t depth;
	uint32_t camFlags;
} GCSpriteData;
extern ComponentID gcSpriteCompID;

typedef struct {
	bool isStencil;
	int stencilID;
} GCStencilData;
extern ComponentID gcStencilCompID;

typedef struct {
	int imgs[9]; // tl, tc, tr, ml, mc, mr, bl, bc, br
	int8_t depth;
	uint32_t camFlags;
} GC3x3SpriteData;
extern ComponentID gc3x3SpriteCompID;

typedef struct {
	Vector2 halfDim;
} GCAABBCollisionData;
extern ComponentID gcAABBCollCompID;

typedef struct {
	float radius;
} GCCircleCollisionData;
extern ComponentID gcCircleCollCompID;

// used for buttons or anything else that should be clickable
typedef struct {
	Vector2 collisionHalfDim;
	int state;
	int camFlags;
	int32_t priority;
	void ( *overResponse )( Entity* btn );
	void ( *leaveResponse )( Entity* btn );
	void ( *pressResponse )( Entity* btn );
	void ( *releaseResponse )( Entity* btn );
} GCPointerResponseData;
extern ComponentID gcPointerResponseCompID;

extern ComponentID gcCleanUpFlagCompID;

// for right now all text will be drawn centered
typedef struct {
	int camFlags;
	int8_t depth;
	const uint8_t* text;
	int fontID;
	Color clr; // assuming any color components aren't for this, need to find a better way to do this
} GCTextData;
extern ComponentID gcTextCompID;

// just a flag you can add to an entity that you can test for later
extern ComponentID gcWatchCompID;

// for entities that are attached to another entity
typedef struct {
	EntityID parentID;
	Vector2 offset;
} GCMountedPosOffset;
extern ComponentID gcMountedPosOffsetCompID;

// for tweening values
typedef struct {
	float duration;
	float timePassed;
	float preDelay;
	float postDelay;
	float startState;
	float endState;
	EaseFunc easing;
} GCFloatTweenData;
extern ComponentID gcAlphaTweenCompID;

typedef struct {
	float duration;
	float timePassed;
	float preDelay;
	float postDelay;
	Vector2 startState;
	Vector2 endState;
	EaseFunc easing;
} GCVec2TweenData;
extern ComponentID gcPosTweenCompID;
extern ComponentID gcScaleTweenCompID;

// attaches the child entity to the parent entity, use the existing positions to calculate the offset
void gc_MountEntity( ECPS* ecps, EntityID parentID, EntityID childID );

void gc_WatchEntity( ECPS* ecps, EntityID id );

void gc_Register( ECPS* ecps );

#endif // inclusion guard