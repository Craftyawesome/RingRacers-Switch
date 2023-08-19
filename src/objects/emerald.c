#include "../k_battle.h"
#include "../k_objects.h"
#include "../info.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../tables.h"

#define emerald_type(o) ((o)->extravalue1)
#define emerald_anim_start(o) ((o)->movedir)
#define emerald_revolution_time(o) ((o)->threshold)
#define emerald_start_radius(o) ((o)->movecount)
#define emerald_target_radius(o) ((o)->extravalue2)
#define emerald_z_shift(o) ((o)->reactiontime)

void Obj_SpawnEmeraldSparks(mobj_t *mobj)
{
	if (leveltime % 3 != 0)
	{
		return;
	}

	mobj_t *sparkle = P_SpawnMobjFromMobj(
		mobj,
		P_RandomRange(PR_SPARKLE, -48, 48) * FRACUNIT,
		P_RandomRange(PR_SPARKLE, -48, 48) * FRACUNIT,
		P_RandomRange(PR_SPARKLE, 0, 64) * FRACUNIT,
		MT_EMERALDSPARK
	);

	sparkle->color = mobj->color;
	sparkle->momz += 8 * mobj->scale * P_MobjFlip(mobj);
	sparkle->sprzoff = mobj->sprzoff;
}

static INT32 get_elapsed(mobj_t *emerald)
{
	return leveltime - min((tic_t)emerald_anim_start(emerald), leveltime);
}

static INT32 get_revolve_time(mobj_t *emerald)
{
	return max(1, emerald_revolution_time(emerald));
}

static fixed_t get_suck_factor(mobj_t *emerald)
{
	const INT32 suck_time = get_revolve_time(emerald) * 2;

	return (min(get_elapsed(emerald), suck_time) * FRACUNIT) / suck_time;
}

static fixed_t get_current_radius(mobj_t *emerald)
{
	fixed_t s = emerald_start_radius(emerald);
	fixed_t t = emerald_target_radius(emerald);

	return s + FixedMul(t - s, get_suck_factor(emerald));
}

static fixed_t get_bob(mobj_t *emerald)
{
	angle_t phase = get_elapsed(emerald) * ((ANGLE_MAX / get_revolve_time(emerald)) / 2);

	return FixedMul(30 * mapobjectscale, FSIN(emerald->angle + phase));
}

static fixed_t center_of(mobj_t *mobj)
{
	return mobj->z + (mobj->height / 2);
}

static fixed_t get_target_z(mobj_t *emerald)
{
	fixed_t shift = FixedMul(emerald_z_shift(emerald), FRACUNIT - get_suck_factor(emerald));

	return center_of(emerald->target) + get_bob(emerald) + shift;
}

static void Obj_EmeraldOrbitPlayer(mobj_t *emerald)
{
	fixed_t r = get_current_radius(emerald);
	fixed_t x = FixedMul(r, FCOS(emerald->angle));
	fixed_t y = FixedMul(r, FSIN(emerald->angle));

	P_MoveOrigin(
			emerald,
			emerald->target->x + x,
			emerald->target->y + y,
			get_target_z(emerald)
	);

	emerald->angle += ANGLE_MAX / get_revolve_time(emerald);
}

void Obj_EmeraldThink(mobj_t *emerald)
{
	if (!P_MobjWasRemoved(emerald->target))
	{
		switch (emerald->target->type)
		{
			case MT_SPECIAL_UFO:
				Obj_UFOEmeraldThink(emerald);
				break;

			case MT_PLAYER:
				Obj_EmeraldOrbitPlayer(emerald);
				break;

			default:
				break;
		}

		return;
	}

	if (emerald->threshold > 0)
	{
		emerald->threshold--;
	}

	A_AttractChase(emerald);

	Obj_SpawnEmeraldSparks(emerald);

	K_BattleOvertimeKiller(emerald);
}

void Obj_BeginEmeraldOrbit(mobj_t *emerald, mobj_t *target, fixed_t radius, INT32 revolution_time)
{
	P_SetTarget(&emerald->target, target);

	emerald_anim_start(emerald) = leveltime;
	emerald_revolution_time(emerald) = revolution_time;

	emerald_start_radius(emerald) = R_PointToDist2(target->x, target->y, emerald->x, emerald->y);
	emerald_target_radius(emerald) = radius;

	emerald->angle = R_PointToAngle2(target->x, target->y, emerald->x, emerald->y);
	emerald_z_shift(emerald) = emerald->z - get_target_z(emerald);

	emerald->flags |= MF_NOGRAVITY | MF_NOCLIP | MF_NOCLIPTHING | MF_NOCLIPHEIGHT;
	emerald->shadowscale = 0;
}
