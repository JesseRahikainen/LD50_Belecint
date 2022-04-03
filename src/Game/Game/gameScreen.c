#include "gameScreen.h"

#include "../Graphics/graphics.h"
#include "../Graphics/images.h"
#include "../Graphics/camera.h"
#include "../Graphics/debugRendering.h"
#include "../Graphics/imageSheets.h"
#include "../UI/text.h"
#include "../System/platformLog.h"
#include "../System/random.h"
#include "../Utils/helpers.h"
#include "../world.h"
#include "../Math/mathUtil.h"
#include "../System/gameTime.h"
#include "../Utils/stretchyBuffer.h"
#include "../Input/input.h"
#include "../sound.h"
#include "../tween.h"
#include "../Graphics/sprites.h"

#include "../System/ECPS/entityComponentProcessSystem.h"

#define WARNING_RADIUS 150.0f
#define PROTECT_RADIUS 60.0f

#define MIN_ARC_HALF_WIDTH 5.0f
#define MAX_ARC_HALF_WIDTH 20.0f

#define MIN_ARC_SPEED_MULT 0.25f
#define MAX_ARC_SPEED_MULT 2.0f

#define MIN_ARC_SPEED 65.0f
#define MAX_ARC_SPEED 85.0f

#define MIN_ARC_HEALTH 3
#define MAX_ARC_HEALTH 8

#define SPEED_LOSS_COUNT_ADJUST 6
#define SPEED_LOSS_FACTOR 0.02f

#define REPULSION_MULTIPLIER 20.0f
#define MIN_INITIAL_REPULSION_MULT 50.0f
#define MAX_INITIAL_REPULSION_MULT 40.0f

#define MIN_ARC_REPEL_DISTANCE 300.0f
#define MAX_ARC_REPEL_DISTANCE 600.0f

#define BPM 150.0f
#define BPS ( BPM / 60.0f )
#define BEAT_LENGTH ( 1 / BPS )

/*static Color bgClr;
static Color centerCircleInnerClr;
static Color centerCircleOuterClr;

static Color warningCircleInnerClr;
static Color warningCircleOuterClr;

static Color dividingLinesClr;
static Color opposingCircleClr;

static Color textClr;//*/

static float prevTotalTime;
static float currTotalTime;
static float totalTime;

static int whiteImg;
static PlatformTexture whiteTex;
static int lineImg;
static int displayFont;
static int textFont;

static int score;

static int currentHitSound;
//            0  1  2  3  4  5  6  7  8  9  10 11 12
//            C  Cs D  Ds E  F  Fs G  Gs A  As B  C
//
// major                   C  D  E  F  G  A  B  C  = 0, 2, 4, 5, 7, 9, 11, 12
// harmonic minor          C  D  Ds F  G  Gs B  C  = 0, 2, 3, 5, 7, 8, 11, 12
// natural minor           C  D  Ds F  G  Gs As C  = 0, 2, 3, 5, 6, 8, 10, 12
// melodic minor up        C  D  Ds F  G  A  B  C  = 0, 2, 3, 5, 7, 9, 11, 12
// melodic minor down      C  D  E  F  G  A  As C  = 0, 2, 4, 5, 7, 9, 10, 12
// dorian                  C  D  Ds F  G  A  As C  = 0, 2, 3, 5, 7, 9, 10, 12
// mixolydian              C  D  E  F  G  A  As C  = 0, 2, 4, 5, 7, 9, 10, 12
// hungarian minor         C  D  Ds Fs G  Gs B  C  = 0, 2, 3, 6, 7, 8, 11, 12
static int sounds[13];
#define NOTES_PER_SCALE 7 // we're ignoring the final C

typedef struct {
	const char* name;
	int notes[NOTES_PER_SCALE];
} Scale;

static Scale* sbScales = NULL;
static int currScale;

static void setupScales( void )
{
	Scale major = { "Major", { 0, 2, 4, 5, 7, 9, 11 } };
	sb_Push( sbScales, major );

	Scale harmonicMinor = { "Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11 } };
	sb_Push( sbScales, harmonicMinor );

	Scale naturalMinor = { "Natural Minor", { 0, 2, 3, 5, 6, 8, 10 } };
	sb_Push( sbScales, naturalMinor );

	Scale melodicMinorUp = { "Melodic Minor Up", { 0, 2, 3, 5, 7, 9, 11 } };
	sb_Push( sbScales, melodicMinorUp );

	Scale melodicMinorDown = { "Melodic Minor Down", { 0, 2, 4, 5, 7, 9, 10 } };
	sb_Push( sbScales, melodicMinorDown );

	Scale dorian = { "Dorian", { 0, 2, 3, 5, 7, 9, 10 } };
	sb_Push( sbScales, dorian );

	Scale mixolydian = { "Mixolydian", { 0, 2, 4, 5, 7, 9, 10 } };
	sb_Push( sbScales, mixolydian );

	Scale hungarianMinor = { "Hungarian Minor", { 0, 2, 3, 6, 7, 8, 11 } };
	sb_Push( sbScales, hungarianMinor );

	currScale = 0;
}

static int chords[][4] = {
	{ 0, 2, -1, -1 }, // these first few aren't actual chords, but i'm too lazy to rename everything
	{ 1, 3, -1, -1 },
	{ 2, 4, -1, -1 },
	{ 3, 5, -1, -1 },
	{ 4, 6, -1, -1 },
	{ 0, 2, 4, -1 },
	{ 1, 3, 5, -1 },
	{ 2, 4, 6, -1 },
	{ 0, 2, 4, 6 }
};

static int introUI;
static int gameOverUI;
static int cloudImg;

static int music;

static float totalTimePassed;

typedef struct {

	float baseAngle;
	float halfArc;

	int resolution;

	float baseDistance;
	float halfWidth;
	float oldBaseDistance;

	float speed;

	float timeToReachCenter;

	float repelDistance;
	Tween repelledMovement;
	bool movingIn;

	int health;

	bool isClicked;

} DangerArc;
static DangerArc* sbArcs = NULL;

typedef struct {
	float angle;
	float width;
} DividingLine;
static DividingLine* sbDivides = NULL;

static float getBeatRemainder( float time )
{
	return fmodf( time, BEAT_LENGTH );
}

static float getCurrentBeatRemainder( )
{
	return fmodf( totalTimePassed, BEAT_LENGTH );
}

static float getCurrentBeat( void )
{
	return totalTimePassed / BEAT_LENGTH;
}

static void startArcMoveIn( DangerArc* arc )
{
	// we want the arc to arrive on the beat
	arc->movingIn = true;

	float beatRemainder = getCurrentBeatRemainder( );

	arc->speed = arc->baseDistance / ( arc->timeToReachCenter - beatRemainder );
}

static void startArcMoveOut( DangerArc* arc )
{
	arc->movingIn = false;
	setTween( &arc->repelledMovement, arc->baseDistance, arc->repelDistance, 1.0f, easeOutCirc );
}

#define FAST_BEAT_BASE 8
#define SLOW_BEAT_BASE 24
#define FAST_BEAT_VAR 2
#define SLOW_BEAT_VAR 8

static size_t addArc( float baseAngle, float halfArc, float val, float startDistance, int resolution, bool movingIn, int health )
{
	DangerArc newArc;

	newArc.baseAngle = baseAngle;
	newArc.halfArc = halfArc;
	newArc.resolution = MAX( resolution, 3 );
	newArc.baseDistance = startDistance;
	newArc.halfWidth = lerp( MIN_ARC_HALF_WIDTH, MAX_ARC_HALF_WIDTH, val );
	newArc.health = health;
	newArc.isClicked = false;

	float t = easeOutCirc( (float)sb_Count( sbArcs ) / 32.0f );
	int baseBeat = (int)lerp( FAST_BEAT_BASE, SLOW_BEAT_BASE, t );
	int beatMod = (int)lerp( FAST_BEAT_VAR, SLOW_BEAT_VAR, t );

	// based on the number of current arcs we'll want to make them slower, so they should take more beats to reach the center
	//  so start at 8 +/- 2, fatter take longer
	int beats = ( ( (int)( ( val - 0.5 ) * beatMod ) ) + baseBeat );
	newArc.timeToReachCenter = BEAT_LENGTH * beats;

	newArc.movingIn = movingIn;

	newArc.repelDistance = lerp( MAX_ARC_REPEL_DISTANCE, MIN_ARC_REPEL_DISTANCE, val );

	if( movingIn ) {
		startArcMoveIn( &newArc );
	} else {
		startArcMoveOut( &newArc );
	}

	sb_Push( sbArcs, newArc );
	return sb_Count( sbArcs ) - 1;
}

static void addLine( float angle )
{
	DividingLine line;
	line.angle = angle;
	line.width = 1.0f;

	sb_Push( sbDivides, line );
}

static void initGame( )
{
	sb_Clear( sbArcs );
	sb_Clear( sbDivides );

	addArc( rand_GetRangeFloat( NULL, 0.0f, M_TWO_PI_F ), M_PI_F, 0.5f, 400.0f, 60, true, 3 );

	score = 0;

	currentHitSound = rand_GetRangeS32( NULL, 0, NOTES_PER_SCALE - 1);
}

typedef struct {
	const char* name;
	Color bgClr;
	Color scoreTextClr;
	Color centerInnerClr;
	Color centerOuterClr;
	Color warningInnerClr;
	Color warningOuterClr;
	Color dividerClr;
	Color opposingCircleClr;
} ColorSetup;

ColorSetup* sbColors = NULL;
static int currColorSetup = 0;

static ColorSetup createColorSetup( const char* name, Color bgClr, Color scoreTextClr, Color centerInnerClr, Color centerOuterClr,
	Color warningClr, Color dividerClr, Color opposingCircleClr )
{
	ColorSetup setup;

	setup.name = name;
	setup.bgClr = bgClr;
	setup.scoreTextClr = scoreTextClr;
	setup.centerInnerClr = centerInnerClr;
	setup.centerOuterClr = centerOuterClr;
	setup.warningInnerClr = setup.warningOuterClr = warningClr;
	setup.warningOuterClr.a = 0.0f;
	setup.dividerClr = dividerClr;
	setup.opposingCircleClr = opposingCircleClr;

	return setup;
}

static void setColors( )
{
	// original colors
	ColorSetup s = createColorSetup( "Original", clr_hex( 0x2B2D42FF ), clr_hex( 0x2B2D42FF ), clr_hex( 0x00FFE2FF ),
		clr_hex( 0x00FFC5FF ), clr_hex( 0xADF5FFFF ), clr_hex( 0xD55672FF ), clr_hex( 0xC42021FF ) );
	sb_Push( sbColors, s );

	s = createColorSetup( "Synth", clr_hex( 0x41277DFF ), clr_hex( 0x41277DFF ), clr_hex( 0x1E873DFF ),
		clr_hex( 0x99C8A7FF ), clr_hex( 0x6EDB8FFF ), clr_hex( 0xB2E340FF ), clr_hex( 0xAD458EFF ) );
	sb_Push( sbColors, s );

	s = createColorSetup( "Sunshine", clr_hex( 0x2D93ADFF ), clr_hex( 0x2D93ADFF ), clr_hex( 0xFFFCC7FF ),
		clr_hex( 0xFFF757FF ), clr_hex( 0xD6C9C9FF ), clr_hex( 0xD6C9C9FF ), clr_hex( 0x37393AFF ) );
	sb_Push( sbColors, s );

	s = createColorSetup( "Night and Blood", clr_hex( 0x2B2C31FF ), clr_hex( 0xDEBA6EFF ), clr_hex( 0x5E2C2DFF ),
		clr_hex( 0x1F0F0FFF ), clr_hex( 0x5E2C2DFF ), clr_hex( 0xFFFAE9FF ), clr_hex( 0xDEBA6EFF ) );
	sb_Push( sbColors, s );

	s = createColorSetup( "Call of the Void", clr_hex( 0x18311DFF ), clr_hex( 0x18311DFF ), clr_hex( 0xA9843CFF ),
		clr_hex( 0x906104FF ), clr_hex( 0x764404FF ), clr_hex( 0x6B8F53FF ), clr_hex( 0x687074FF ) ); // https://www.youtube.com/watch?v=wXRaPlvFvuk
	sb_Push( sbColors, s );

	s = createColorSetup( "Vintage", clr_hex( 0x5a3d2bFF ), clr_hex( 0x5a3d2bFF ),
		clr_hex( 0xe5771eFF ), clr_hex( 0xf4a127FF ),
		clr_hex( 0xffecb4FF ), clr_hex( 0xffecb4FF ), clr_hex( 0x75c8aeFF ) );
	sb_Push( sbColors, s );

	s = createColorSetup( "Dungeon", clr_hex( 0x000000FF ), // bg
		clr_hex( 0x000000FF ), // text
		clr_hex( 0x444444FF ), clr_hex( 0xFF0000FF ), // center inner, outer
		clr_hex( 0xFFFFFFFF ), // warning
		clr_hex( 0xFF0000FF ), // divider
		clr_hex( 0xEEEEEEFF ) ); // opposition
	sb_Push( sbColors, s );
}

static void drawRingArc( float innerRadius, float outerRadius, float startAngleRad, float endAngleRad, Color clr, int resolution )
{
	Vector2 firstInnerPos, firstOuterPos;
	vec2_FromPolar( startAngleRad, innerRadius, &firstInnerPos );
	vec2_FromPolar( startAngleRad, outerRadius, &firstOuterPos );

	Vector2 prevInnerPos = firstInnerPos;
	Vector2 prevOuterPos = firstOuterPos;

	float step = SDL_fabsf( ( endAngleRad - startAngleRad ) / resolution );

	TriVert verts[3];
	for( int i = 1; i <= resolution; ++i ) {
		float angle = radianRotWrap( startAngleRad + i * step );

		Vector2 currInnerPos;
		vec2_FromPolar( angle, innerRadius, &currInnerPos );

		Vector2 currOuterPos;
		vec2_FromPolar( angle, outerRadius, &currOuterPos );

		verts[0].pos = prevOuterPos;
		verts[0].col = clr;
		verts[0].uv = VEC2_ZERO;

		verts[1].pos = currOuterPos;
		verts[1].col = clr;
		verts[1].uv = VEC2_ZERO;

		verts[2].pos = prevInnerPos;
		verts[2].col = clr;
		verts[2].uv = VEC2_ZERO;

		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTex, whiteTex, 0.0f, -1, 1, -1, TT_TRANSPARENT );

		verts[0].pos = currOuterPos;
		verts[0].col = clr;
		verts[0].uv = VEC2_ZERO;

		verts[1].pos = currInnerPos;
		verts[1].col = clr;
		verts[1].uv = VEC2_ZERO;

		verts[2].pos = prevInnerPos;
		verts[2].col = clr;
		verts[2].uv = VEC2_ZERO;

		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTex, whiteTex, 0.0f, -1, 1, -1, TT_TRANSPARENT );

		prevInnerPos = currInnerPos;
		prevOuterPos = currOuterPos;
	}
}

static float noNoiseFunction( float angle, float radius )
{
	return radius;
}

#define BASE_NOISE_ANGLE_MOD 50.0f
#define BASE_NOISE_TIME_MOD 0.1f
#define BASE_NOISE_SINE_STR 5.0f

#define MIN_NOISE_ANGLE_MOD 5.0f
#define MAX_NOISE_ANGLE_MOD 10.0f
#define MIN_NOISE_TIME_MOD 0.1f
#define MAX_NOISE_TIME_MOD 0.1f
#define MIN_NOISE_SINE_STR 10.0f
#define MAX_NOISE_SINE_STR 15.0f

#define NOISE_ANGLE_MOD_LERP 10.0f
#define NOISE_TIME_MOD_LERP 10.0f
#define NOISE_SINE_STR_LERP 10.0f


static float noiseAngleMod = BASE_NOISE_ANGLE_MOD;
static float noiseTimeMod = BASE_NOISE_TIME_MOD;
static float noiseSineStr = BASE_NOISE_SINE_STR;
static float noiseRadius = PROTECT_RADIUS;

static void processNoiseValues( float dt )
{
	noiseAngleMod = lerp( noiseAngleMod, BASE_NOISE_ANGLE_MOD, dt * NOISE_ANGLE_MOD_LERP );
	noiseTimeMod = lerp( noiseTimeMod, BASE_NOISE_TIME_MOD, dt * NOISE_TIME_MOD_LERP );
	noiseSineStr = lerp( noiseSineStr, BASE_NOISE_SINE_STR, dt * NOISE_SINE_STR_LERP );
}

static float circleNoiseFunction( float angle, float radius )
{
	return radius + ( SDL_sinf( noiseAngleMod * ( angle + ( totalTime * noiseTimeMod ) ) ) * noiseSineStr );
}

static void drawCircle( float radius, Color centerClr, Color outerColor, int resolution, int8_t depth, float(*noiseFunc)( float, float ) )
{
	float step = M_TWO_PI_F / (float)resolution;

	Vector2 firstPos;
	vec2_FromPolar( 0.0f, noiseFunc( 0.0f, radius ), &firstPos );

	Vector2 prevPos = firstPos;
	
	TriVert verts[3];
	for( int i = 1; i < resolution; ++i ) {
		float angle = i * step;
		Vector2 currPos;
		vec2_FromPolar( angle, noiseFunc( angle, radius ), &currPos );

		verts[0].pos = VEC2_ZERO;
		verts[0].col = centerClr;
		verts[0].uv = VEC2_ZERO;

		verts[1].pos = currPos;
		verts[1].col = outerColor;
		verts[1].uv = VEC2_ZERO;

		verts[2].pos = prevPos;
		verts[2].col = outerColor;
		verts[2].uv = VEC2_ZERO;

		triRenderer_AddVertices( verts, ST_DEFAULT, whiteTex, whiteTex, 0.0f, -1, 1, depth, TT_TRANSPARENT );

		prevPos = currPos;
	}

	verts[0].pos = VEC2_ZERO;
	verts[0].col = centerClr;
	verts[0].uv = VEC2_ZERO;

	verts[1].pos = firstPos;
	verts[1].col = outerColor;
	verts[1].uv = VEC2_ZERO;

	verts[2].pos = prevPos;
	verts[2].col = outerColor;
	verts[2].uv = VEC2_ZERO;

	triRenderer_AddVertices( verts, ST_DEFAULT, whiteTex, whiteTex, 0.0f, -1, 1, depth, TT_TRANSPARENT );
}

static void gfxDrawDangerArcs( float t )
{
	for( size_t i = 0; i < sb_Count( sbArcs ); ++i ) {
		float dist = lerp( sbArcs[i].oldBaseDistance, sbArcs[i].baseDistance, t );
		float halfWidth = sbArcs[i].halfWidth;
		if( !sbArcs[i].movingIn ) {
			float t = sbArcs[i].repelledMovement.elapsed / sbArcs[i].repelledMovement.duration;
			halfWidth *= lerp( 0.1f, 1.0f, t );
		}
		drawRingArc( dist - halfWidth, dist + halfWidth,
			sbArcs[i].baseAngle - sbArcs[i].halfArc, sbArcs[i].baseAngle + sbArcs[i].halfArc,
			sbColors[currColorSetup].opposingCircleClr, sbArcs[i].resolution );
	}
}

static void gfxSwapDangerArcs( void )
{
	for( size_t i = 0; i < sb_Count( sbArcs ); ++i ) {
		sbArcs[i].oldBaseDistance = sbArcs[i].baseDistance;
	}
}

static void gfxDrawCenterCircle( float t )
{
	totalTime = lerp( prevTotalTime, currTotalTime, t );

	drawCircle( WARNING_RADIUS, sbColors[currColorSetup].warningInnerClr, sbColors[currColorSetup].warningOuterClr, 60, -2, noNoiseFunction );
	drawCircle( PROTECT_RADIUS, sbColors[currColorSetup].centerInnerClr, sbColors[currColorSetup].centerOuterClr, 60, 0, circleNoiseFunction );
}

static void playHitSound( int i )
{
	snd_Play( sounds[sbScales[currScale].notes[i]], 0.5f, 1.0f, 0.0f, 0 );
}

static void playChord( int c )
{
	for( size_t i = 0; i < ARRAY_SIZE( chords[c] ); ++i ) {
		if( chords[c][i] == -1 ) continue;
		playHitSound( chords[c][i] );
	}
}

static int* sbChosenChords = NULL;
static void playRandomChord( void )
{
	// find a chord that contains the the note, or a note within a step of the last note
	sb_Clear( sbChosenChords );
	for( size_t i = 0; i < ARRAY_SIZE( chords ); ++i ) {
		bool isValid = false;
		for( size_t a = 0; ( a < 4 ) && !isValid; ++a ) {
			if( ( chords[i][a] == currentHitSound ) || ( chords[i][a] == ( currentHitSound - 1 ) ) || ( chords[i][a] == ( currentHitSound + 1 ) ) ) {
				isValid = true;
			}
		}

		if( isValid ) {
			sb_Push( sbChosenChords, i );
		}
	}

	if( sb_Count( sbChosenChords ) == 0 ) {
		// fall back
		int rand = rand_GetRangeS32( NULL, 0, ARRAY_SIZE( chords ) - 1 );
		sb_Push( sbChosenChords, rand );
	}

	playChord( sbChosenChords[rand_GetRangeS32( NULL, 0, sb_Count( sbChosenChords ) - 1 )] );// rand_GetRangeS32( NULL, 0, ARRAY_SIZE( chords ) - 1 ) );
}

static void chooseNewHitSound( )
{
	// play repulsion sound
	float chance = rand_GetNormalizedFloat( NULL );

	// small chance it's stays the same
	if( chance < 0.05f ) {
		// do nothing
	} else if( chance < 85.0f ) {
		// larger chance of single step
		int delta = rand_Choice( NULL ) ? 1 : -1;
		if( ( ( currentHitSound + delta ) < 0 ) || ( ( currentHitSound + delta ) >= NOTES_PER_SCALE ) ) {
			delta *= -1;
		}
		currentHitSound += delta;
	} else {
		// small chance of random
		currentHitSound = rand_GetRangeS32( NULL, 0, NOTES_PER_SCALE - 1 );
	}
}

// returns if the arc is destroyed
static bool repulseArc( size_t arcIdx )
{
	//noiseAngleMod += rand_GetRangeFloat( NULL, MIN_NOISE_ANGLE_MOD, MAX_NOISE_ANGLE_MOD );
	noiseTimeMod += rand_GetRangeFloat( NULL, MIN_NOISE_TIME_MOD, MAX_NOISE_TIME_MOD ) * ( rand_Choice( NULL ) ? -1.0f : 1.0f );
	noiseSineStr += rand_GetRangeFloat( NULL, MIN_NOISE_SINE_STR, MAX_NOISE_SINE_STR );

	startArcMoveOut( &sbArcs[arcIdx] );

	++score;

	--sbArcs[arcIdx].health;
	if( sbArcs[arcIdx].health <= 0 ) {
		DangerArc oldArc = sbArcs[arcIdx];
		sb_Remove( sbArcs, arcIdx );

		// split arc in half
		// create new arcs
		int repulsionChoice = rand_Choice( NULL ) ? 0 : 1;
		addArc( radianRotWrap( oldArc.baseAngle - oldArc.halfArc / 2.0f ), oldArc.halfArc / 2.0f, rand_GetNormalizedFloat( NULL ),
			oldArc.baseDistance, oldArc.resolution / 2, false, rand_GetRangeS32( NULL, MIN_ARC_HEALTH, MAX_ARC_HEALTH ) );

		addArc( radianRotWrap( oldArc.baseAngle + oldArc.halfArc / 2.0f ), oldArc.halfArc / 2.0f, rand_GetNormalizedFloat( NULL ),
			oldArc.baseDistance, oldArc.resolution / 2, false, rand_GetRangeS32( NULL, MIN_ARC_HEALTH, MAX_ARC_HEALTH ) );

		// create new dividing line(s)
		if( sb_Count( sbArcs ) == 2 ) {
			// initial split, need to create two lines
			addLine( oldArc.baseAngle  );
			addLine( oldArc.baseAngle - oldArc.halfArc );
		} else {
			// create one line
			addLine( oldArc.baseAngle );
		}

		playRandomChord( );

		return true;
	} else {
		chooseNewHitSound( );
		playHitSound( currentHitSound );

		return false;
	}
}

static void processArcInputs( void )
{
	for( size_t i = 0; i < sb_Count( sbArcs ); ++i ) {
		if( sbArcs[i].isClicked ) {
			sbArcs[i].isClicked = false;
			if( repulseArc( i ) ) {
				--i;
			}
		}
	}
}

typedef struct {
	float timeAlive;
	float currBottom;
} InfoDisplayHide;
InfoDisplayHide colorInfoHide;
InfoDisplayHide scaleInfoHide;

static void updateInfo( InfoDisplayHide* info, float dt )
{
	info->timeAlive += dt;
	if( info->timeAlive <= 1.0f ) {
		info->currBottom = 390.0f;
	} else {
		info->currBottom = lerp( 390.0f, 440.0f, easeInQuad( clamp( 0.0f, 1.0f, ( info->timeAlive - 1.0f ) * 1.25f ) ) );
	}
}

static void updateInfoTimeAlive( float dt )
{
	updateInfo( &colorInfoHide, dt );
	updateInfo( &scaleInfoHide, dt );
}

static void resetInfo( InfoDisplayHide* info )
{
	info->timeAlive = 0.0f;
	info->currBottom = 390.0f;
}

static void resetInfoTimeAlive( void )
{
	resetInfo( &colorInfoHide );
	resetInfo( &scaleInfoHide );
}

static void drawColorInfo( void )
{
	char txt[128];
	SDL_snprintf( txt, 128, "'q' and 'w' to cycle palettes\nCurrent palette: %s", sbColors[currColorSetup].name );
	txt_DisplayString( txt, vec2( -350.0f, colorInfoHide.currBottom ), CLR_WHITE, HORIZ_ALIGN_LEFT, VERT_ALIGN_BOTTOM, textFont, 1, 110, 18.0f );
}

static void nextColorConfig( void )
{
	++currColorSetup;
	if( currColorSetup >= (int)sb_Count( sbColors ) ) {
		currColorSetup = 0;
	}

	gfx_SetClearColor( sbColors[currColorSetup].bgClr );

	resetInfo( &colorInfoHide );
}

static void prevColorConfig( void )
{
	--currColorSetup;
	if( currColorSetup < 0  ) {
		currColorSetup = sb_Count( sbColors ) - 1;
	}

	gfx_SetClearColor( sbColors[currColorSetup].bgClr );

	resetInfo( &colorInfoHide );
}

static bool playOnChordChange = true;
static void nextScale( void )
{
	++currScale;
	if( currScale >= (int)sb_Count( sbScales ) ) {
		currScale = 0;
	}
	if( playOnChordChange ) {
		playChord( 7 );
	}

	resetInfo( &scaleInfoHide );
}

static void prevScale( void )
{
	--currScale;
	if( currScale < 0 ) {
		currScale = sb_Count( sbScales ) - 1;
	}
	if( playOnChordChange ) {
		playChord( 7 );
	}

	resetInfo( &scaleInfoHide );
}

static void drawScaleInfo( void )
{
	char txt[128];
	SDL_snprintf( txt, 128, "'a' and 's' to cycle scales\nCurrent scale: %s", sbScales[currScale].name );
	txt_DisplayString( txt, vec2( 350.0f, scaleInfoHide.currBottom ), CLR_WHITE, HORIZ_ALIGN_RIGHT, VERT_ALIGN_BOTTOM, textFont, 1, 110, 18.0f );
}

// **********************
//  Game states
static int intro_Enter( void );
static int intro_Exit( void );
static void intro_ProcessEvents( SDL_Event* evt );
static void intro_Process( void );
static void intro_Draw( void );
static void intro_PhysicsTick( float dt );
static int play_Enter( void );
static int play_Exit( void );
static void play_ProcessEvents( SDL_Event* evt );
static void play_Process( void );
static void play_Draw( void );
static void play_PhysicsTick( float dt );
static int gameOver_Enter( void );
static int gameOver_Exit( void );
static void gameOver_ProcessEvents( SDL_Event* evt );
static void gameOver_Process( void );
static void gameOver_Draw( void );
static void gameOver_PhysicsTick( float dt );
GameState introState = { intro_Enter, intro_Exit, intro_ProcessEvents, intro_Process, intro_Draw, intro_PhysicsTick };
GameState playState = { play_Enter, play_Exit, play_ProcessEvents, play_Process, play_Draw, play_PhysicsTick };
GameState gameOverState = { gameOver_Enter, gameOver_Exit, gameOver_ProcessEvents, gameOver_Process, gameOver_Draw, gameOver_PhysicsTick };

static GameStateMachine stateMachine;
static float timer = 0.0f;

typedef struct {
	EntityID sprite;
	float rotationVel;
} BGCloud;

static BGCloud* sbClouds;

static void createCloud( Vector2 pos, float scale, float rot, float alpha )
{
	BGCloud newCloud;

	newCloud.sprite = spr_CreateSprite( cloudImg, 1, pos, vec2( scale, scale ), rand_GetRangeFloat( NULL, 0.0f, M_TWO_PI_F ), clr( 1.0f, 1.0f, 1.0f, alpha ), -110 );
	newCloud.rotationVel = rot;

	sb_Push( sbClouds, newCloud );
}

static void updateClouds( float dt )
{
	float rotAdj = lerp( 1.0f, 4.0f, ( sb_Count( sbArcs ) / 10.0f ) );
	for( size_t i = 0; i < sb_Count( sbClouds ); ++i ) {
		spr_UpdateDelta( sbClouds[i].sprite, &VEC2_ZERO, &VEC2_ZERO, ( sbClouds[i].rotationVel * rotAdj ) * dt );
	}
}

static int gameScreen_Enter( void )
{
	setColors( );
	setupScales( );

	Vector2 worldSize;
	world_GetSize( &worldSize );
	cam_SetCenteredProjectionMatrix( 0, (int)worldSize.w, (int)worldSize.h );

	cam_TurnOnFlags( 0, 1 );
	
	gfx_SetClearColor( sbColors[currColorSetup].bgClr );

	whiteImg = img_Load( "Images/white.png", ST_DEFAULT );
	img_GetTextureID( whiteImg, &whiteTex );

	lineImg = img_Load( "Images/line.png", ST_DEFAULT );
	img_SetRatioOffset( lineImg, vec2( 0.5f, 1.0f ), VEC2_ZERO );

	gfx_AddDrawTrisFunc( gfxDrawCenterCircle );
	gfx_AddDrawTrisFunc( gfxDrawDangerArcs );
	gfx_AddClearCommand( gfxSwapDangerArcs );

	prevTotalTime = rand_GetRangeFloat( NULL, 0, 30.0f );

	displayFont = txt_CreateSDFFont( "Fonts/NotoSans-Bold.ttf" );
	textFont = txt_CreateSDFFont( "Fonts/NotoSans-Regular.ttf" );

	sounds[0] = snd_LoadSample( "sounds/blipC2.ogg", 1, false );
	sounds[1] = snd_LoadSample( "sounds/blipCs2.ogg", 1, false );
	sounds[2] = snd_LoadSample( "sounds/blipD2.ogg", 1, false );
	sounds[3] = snd_LoadSample( "sounds/blipDs2.ogg", 1, false );
	sounds[4] = snd_LoadSample( "sounds/blipE2.ogg", 1, false );
	sounds[5] = snd_LoadSample( "sounds/blipF2.ogg", 1, false );
	sounds[6] = snd_LoadSample( "sounds/blipFs2.ogg", 1, false );
	sounds[7] = snd_LoadSample( "sounds/blipG2.ogg", 1, false );
	sounds[8] = snd_LoadSample( "sounds/blipGs2.ogg", 1, false );
	sounds[9] = snd_LoadSample( "sounds/blipA2.ogg", 1, false );
	sounds[10] = snd_LoadSample( "sounds/blipAs2.ogg", 1, false );
	sounds[11] = snd_LoadSample( "sounds/blipB2.ogg", 1, false );
	sounds[12] = snd_LoadSample( "sounds/blipC3.ogg", 1, false );

	int* tempIDs = NULL;
	img_LoadSpriteSheet( "Images/ui.ss", ST_OUTLINED_IMAGE_SDF, &tempIDs );
	sb_Release( tempIDs );
	introUI = img_GetExistingByID( "intro.png" );
	gameOverUI = img_GetExistingByID( "gameOver.png" );
	timer = 0.0f;
	totalTimePassed = 0.0f;

	spr_Init( );

	cloudImg = img_Load( "Images/cloud.png", ST_DEFAULT );

	createCloud( VEC2_ZERO, 4.0f, 0.0f, 0.025f );
	createCloud( VEC2_ZERO, 5.0f, 0.025f, 0.025f );
	createCloud( VEC2_ZERO, 6.0f, -0.025f, 0.025f );

	createCloud( vec2( worldSize.w / 4.0f, worldSize.h / 4.0f ), 2.0f, -0.1f, .025f );
	createCloud( vec2( worldSize.w / 4.0f, worldSize.h / 4.0f ), 2.0f, 0.1f, .025f );

	createCloud( vec2( -worldSize.w / 4.0f, worldSize.h / 4.0f ), 2.0f, -0.1f, .025f );
	createCloud( vec2( -worldSize.w / 4.0f, worldSize.h / 4.0f ), 2.0f, 0.1f, .025f );

	createCloud( vec2( -worldSize.w / 4.0f, -worldSize.h / 4.0f ), 2.0f, -0.1f, .025f );
	createCloud( vec2( -worldSize.w / 4.0f, -worldSize.h / 4.0f ), 2.0f, 0.1f, .025f );

	createCloud( vec2( worldSize.w / 4.0f, -worldSize.h / 4.0f ), 2.0f, -0.1f, .025f );
	createCloud( vec2( worldSize.w / 4.0f, -worldSize.h / 4.0f ), 2.0f, 0.1f, .025f );//*/

	gsm_EnterState( &stateMachine, &introState );

	input_BindOnKeyPress( SDLK_q, prevColorConfig );
	input_BindOnKeyPress( SDLK_w, nextColorConfig );

	input_BindOnKeyPress( SDLK_a, prevScale );
	input_BindOnKeyPress( SDLK_s, nextScale );

	return 1;
}

static int gameScreen_Exit( void )
{
	return 1;
}

static void gameScreen_ProcessEvents( SDL_Event* e )
{
	gsm_ProcessEvents( &stateMachine, e );
}

#define PROCESS_DELAY ( BEAT_LENGTH / 2.0f )
static void gameScreen_Process( void )
{
	float dt = gt_GetRenderTimeDelta( );
	float prevTimer = timer;
	timer += dt;
	totalTimePassed += dt;

	if( ( prevTimer <= PROCESS_DELAY ) && ( timer >= PROCESS_DELAY ) ) {
		processArcInputs( );
		while( timer >= PROCESS_DELAY ) {
			timer -= PROCESS_DELAY;
		}
	}

	gsm_Process( &stateMachine );
}

static void drawDividingLine( DividingLine* line )
{
	int draw = img_CreateDraw( lineImg, 1, VEC2_ZERO, VEC2_ZERO, -100 );
	float newWidth = rand_GetToleranceFloat( NULL, 1.0f, 0.1f );
	img_SetDrawScaleV( draw, vec2( line->width, 100.0f ), vec2( line->width, 100.0f ) );
	line->width = newWidth;
	img_SetDrawRotation( draw, line->angle, line->angle );
	img_SetDrawColor( draw, sbColors[currColorSetup].dividerClr, sbColors[currColorSetup].dividerClr );
}

static void gameScreen_Draw( void )
{
	prevTotalTime = currTotalTime;
	currTotalTime = currTotalTime + gt_GetRenderTimeDelta( );
	
	for( size_t i = 0; i < sb_Count( sbDivides ); ++i ) {
		drawDividingLine( &( sbDivides[i] ) );
	}

	char text[32];
	SDL_snprintf( text, 32, "%i", score );
	txt_DisplayString( text, VEC2_ZERO, sbColors[currColorSetup].scoreTextClr, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFont, 1, 50, 60.0f );

	drawColorInfo( );
	drawScaleInfo( );

	gsm_Draw( &stateMachine );
}

static void arcPhysics( float dt )
{
	// process arcs
	int32_t numArcs = (int32_t)sb_Count( sbArcs ) - SPEED_LOSS_COUNT_ADJUST;
	float lossScale = SPEED_LOSS_FACTOR * numArcs;
	float speedLoss = 1.0f - ( lossScale / SDL_sqrtf( ( lossScale * lossScale ) + 1 ) );
	for( size_t i = 0; i < sb_Count( sbArcs ); ++i ) {
		DangerArc* a = &( sbArcs[i] );

		if( a->movingIn ) {
			// use linear inwards movement
			a->baseDistance -= a->speed * dt;
		} else {
			// use exponential outwards movement
			bool done = processTween( &a->repelledMovement, dt );
			a->baseDistance = a->repelledMovement.current;
			if( done ) {
				startArcMoveIn( a );
			}
		}

		if( a->baseDistance <= 0.0f ) {
			a->baseDistance = 0.0f;
		}

		if( !a->isClicked && ( a->baseDistance <= 0.1f ) ) {
			// game over
			gsm_EnterState( &stateMachine, &gameOverState );
		}
	}
}

static void gameScreen_PhysicsTick( float dt )
{
	updateClouds( dt );

	processNoiseValues( dt );
	gsm_PhysicsTick( &stateMachine, dt );
}

// *********************
//  Intro state - just draw the circle and the instructions
static void introMouseClick( void )
{
	gsm_EnterState( &stateMachine, &playState );
}

static int intro_Enter( void )
{
	resetInfoTimeAlive( );

	playOnChordChange = true;
	initGame( );
	input_BindOnMouseButtonPress( SDL_BUTTON_LEFT, introMouseClick );
	return 1;
}

static int intro_Exit( void )
{
	input_ClearMouseButtonBinds( SDL_BUTTON_LEFT );
	return 1;
}

static void intro_ProcessEvents( SDL_Event* evt )
{

}

static void intro_Process( void )
{

}

static void intro_Draw( void )
{
	int draw = img_CreateDraw( introUI, 1, vec2( 1.5f, -184.5 ), vec2( 1.5f, -184.5 ), 100 );
}

static void intro_PhysicsTick( float dt )
{
}

// *********************
//  Play state - play the game
#define CLICK_ANGLE_BUFFER DEG_TO_RAD( 2.0f )
static size_t* sbClicked = NULL;

static void playMouseClick( void )
{
	// check the angle from the center, if it matches any arcs, if that arc is in the warning zone, and if it's currently moving inwards
	//  if it is then repel it
	Vector2 clickPos;
	input_GetMousePosition( &clickPos );
	Vector2 worldSize;
	world_GetSize( &worldSize );
	vec2_Scale( &worldSize, 0.5f, &worldSize );

	vec2_Subtract( &clickPos, &worldSize, &clickPos );

	float angle = vec2_RotationRadians( &clickPos );
	angle = radianRotWrap( angle );
	//llog( LOG_DEBUG, "angle: %f", RAD_TO_DEG( angle ) );

	sb_Clear( sbClicked );
	for( size_t i = 0; i < sb_Count( sbArcs ); ++i ) {

		if( ( sbArcs[i].baseDistance - sbArcs[i].halfWidth ) > WARNING_RADIUS ) continue;
		if( !sbArcs[i].movingIn ) continue;

		// check to see if it's in the arc
		float dist = radianRotDiff( sbArcs[i].baseAngle, angle );
		//llog( LOG_DEBUG, "diff: %f", RAD_TO_DEG( dist ) );

		if( fabsf( dist ) <= ( sbArcs[i].halfArc + CLICK_ANGLE_BUFFER ) ) {
			//llog( LOG_DEBUG, "Found %i", i );
			//sbArcs[i].isClicked = true;
			//repulseArc( i );
			sb_Push( sbClicked, i );
		}
	}

	// find the valid click that is the closest to the center and click them
	size_t closest = SIZE_MAX;
	float closestDist = 10000.0f;
	for( size_t i = 0; i < sb_Count( sbClicked ); ++i ) {
		if( sbArcs[sbClicked[i]].baseDistance < closestDist ) {
			closest = sbClicked[i];
			closestDist = sbArcs[sbClicked[i]].baseDistance;
		}
	}

	if( closest != SIZE_MAX ) {
		sbArcs[closest].isClicked = true;
	}
}

static int play_Enter( void )
{
	playOnChordChange = false;
	input_BindOnMouseButtonPress( SDL_BUTTON_LEFT, playMouseClick );
	return 1;
}

static int play_Exit( void )
{
	input_ClearMouseButtonBinds( SDL_BUTTON_LEFT );
	return 1;
}

static void play_ProcessEvents( SDL_Event* evt )
{

}

static void play_Process( void )
{

}

static void play_Draw( void )
{

}

static void play_PhysicsTick( float dt )
{
	arcPhysics( dt );
	updateInfoTimeAlive( dt );
}

// *********************
//  Game over state - show the summary
static float timeInGameOver;
static void gameOverMouseClick( void )
{
	if( timeInGameOver <= 0.5f ) return;
	gsm_EnterState( &stateMachine, &introState );
}

static int gameOver_Enter( void )
{
	resetInfoTimeAlive( );
	playOnChordChange = true;
	timeInGameOver = 0.0f;
	input_BindOnMouseButtonPress( SDL_BUTTON_LEFT, gameOverMouseClick );
	return 1;
}

static int gameOver_Exit( void )
{
	input_ClearMouseButtonBinds( SDL_BUTTON_LEFT );
	return 1;
}

static void gameOver_ProcessEvents( SDL_Event* evt )
{
	
}

static void gameOver_Process( void )
{

}

static void gameOver_Draw( void )
{
	int draw = img_CreateDraw( gameOverUI, 1, VEC2_ZERO, VEC2_ZERO, 100 );
}

static void gameOver_PhysicsTick( float dt )
{
	timeInGameOver += dt;
}


GameState gameScreenState = { gameScreen_Enter, gameScreen_Exit, gameScreen_ProcessEvents,
	gameScreen_Process, gameScreen_Draw, gameScreen_PhysicsTick };