#include "stdafx.h"
#include "..\igame_persistent.h"
#include "..\environment.h"

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3		corners [8]			= {
	{ -1, -1,  0.7 },	{ -1, -1, +1},
	{ -1, +1, +1 },		{ -1, +1,  0.7},
	{ +1, +1, +1 },		{ +1, +1,  0.7},
	{ +1, -1, +1 },		{ +1, -1,  0.7}
};
static u16			facetable[16][3]		= {
	{ 3, 2, 1 },  
	{ 3, 1, 0 },		
	{ 7, 6, 5 }, 
	{ 5, 6, 4 },		
	{ 3, 5, 2 },
	{ 4, 2, 5 },		
	{ 1, 6, 7 },
	{ 7, 0, 1 },

	{ 5, 3, 0 },
	{ 7, 5, 0 },

	{ 1, 4, 6 },
	{ 2, 4, 1 },
};

void CRenderTarget::accum_direct(u32 sub_phase)
{
	// Choose normal code-path or filtered
	phase_accumulator();
	if (RImplementation.o.sunfilter) {
		accum_direct_f(sub_phase);
		return;
	}

	// *** assume accumulator setted up ***
	light* fuckingsun = (light*)RImplementation.Lights.sun_adapted._get();

	// Common calc for quad-rendering
	u32		Offset;
	u32		C = color_rgba(255, 255, 255, 255);
	float	_w = float(Device.dwWidth);
	float	_h = float(Device.dwHeight);
	Fvector2					p0, p1;
	p0.set(.5f / _w, .5f / _h);
	p1.set((_w + .5f) / _w, (_h + .5f) / _h);
	float	d_Z = EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir, L_clr;	float L_spec;
	L_clr.set(fuckingsun->color.r, fuckingsun->color.g, fuckingsun->color.b);
	L_spec = u_diffuse2s(L_clr);
	Device.mView.transform_dir(L_dir, fuckingsun->direction);
	L_dir.normalize();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode(CULL_NONE);
	if (SE_SUN_NEAR == sub_phase)	//.
	{
		// Fill vertex buffer
		FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, Offset);
		pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);	pv++;
		pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);	pv++;
		pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);	pv++;
		pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);	pv++;
		RCache.Vertex.Unlock(4, g_combine->vb_stride);
		RCache.set_Geometry(g_combine);

		// setup
		float	intensity = 0.3f * fuckingsun->color.r + 0.48f * fuckingsun->color.g + 0.22f * fuckingsun->color.b;
		Fvector	dir = L_dir;
		dir.normalize().mul(-_sqrt(intensity + EPS));
		RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
		RCache.set_c("Ldynamic_dir", dir.x, dir.y, dir.z, 0);

		// if (stencil>=1 && aref_pass)	stencil = light_id
		RCache.set_ColorWriteEnable(FALSE);
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r2_sun_near);
	Device.mFullTransform.transform(center_pt);
	d_Z = center_pt.z;

	// nv-stencil recompression
	if (RImplementation.o.nvstencil && (SE_SUN_NEAR == sub_phase))	u_stencil_optimize();	//. driver bug?

	// Perform lighting
	{
		phase_accumulator();
		RCache.set_CullMode(CULL_NONE);
		RCache.set_ColorWriteEnable();

		// texture adjustment matrix
		float			fTexelOffs = (.5f / float(RImplementation.o.smapsize));
		float			fRange = (SE_SUN_NEAR == sub_phase) ? ps_r2_sun_depth_near_scale : ps_r2_sun_depth_far_scale;
		float			fBias = (SE_SUN_NEAR == sub_phase) ? ps_r2_sun_depth_near_bias : ps_r2_sun_depth_far_bias;
		Fmatrix			m_TexelAdjust =
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				fRange,			0.0f,
			0.5f + fTexelOffs,	0.5f + fTexelOffs,	fBias,			1.0f
		};

		// compute xforms
		FPU::m64r();
		Fmatrix				xf_invview;		xf_invview.invert(Device.mView);

		// shadow xform
		Fmatrix				m_shadow;
		{
			Fmatrix			xf_project;		xf_project.mul(m_TexelAdjust, fuckingsun->X.D.combine);
			m_shadow.mul(xf_project, xf_invview);

			// tsm-bias
			if ((SE_SUN_FAR == sub_phase) && (RImplementation.o.HW_smap))
			{
				Fvector		bias;	bias.mul(L_dir, ps_r2_sun_tsm_bias);
				Fmatrix		bias_t;	bias_t.translate(bias);
				m_shadow.mulB_44(bias_t);
			}
			FPU::m24r();
		}

		// clouds xform
		Fmatrix				m_clouds_shadow;
		{
			static	float	w_shift = 0;
			Fmatrix			m_xform;
			Fvector			direction = fuckingsun->direction;
			float	w_dir = g_pGamePersistent->Environment.CurrentEnv.wind_direction;
			//float	w_speed				= g_pGamePersistent->Environment.CurrentEnv.wind_velocity	;
			Fvector			normal;	normal.setHP(w_dir, 0);
			w_shift += 0.003f * Device.fTimeDelta;
			Fvector			position;	position.set(0, 0, 0);
			m_xform.build_camera_dir(position, direction, normal);
			Fvector			localnormal; m_xform.transform_dir(localnormal, normal); localnormal.normalize();
			m_clouds_shadow.mul(m_xform, xf_invview);
			m_xform.scale(0.002f, 0.002f, 1.f);
			m_clouds_shadow.mulA_44(m_xform);
			m_xform.translate(localnormal.mul(w_shift));
			m_clouds_shadow.mulA_44(m_xform);
		}

		// Make jitter texture
		Fvector2					j0, j1;
		float	scale_X = float(Device.dwWidth) / float(TEX_jitter);
		//float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
		float	offset = (.5f / float(TEX_jitter));
		j0.set(offset, offset);
		j1.set(scale_X, scale_X).add(offset);

		// Fill vertex buffer
		FVF::TL2uv* pv = (FVF::TL2uv*)RCache.Vertex.Lock(4, g_combine_2UV->vb_stride, Offset);
		pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y, j0.x, j1.y);	pv++;
		pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y, j0.x, j0.y);	pv++;
		pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y, j1.x, j1.y);	pv++;
		pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y, j1.x, j0.y);	pv++;
		RCache.Vertex.Unlock(4, g_combine_2UV->vb_stride);
		RCache.set_Geometry(g_combine_2UV);

		// setup
		RCache.set_Element(s_accum_direct->E[sub_phase]);
		RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
		RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
		RCache.set_c("m_shadow", m_shadow);
		RCache.set_c("m_sunmask", m_clouds_shadow);

		// setup stencil
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
		RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
	}

}
void CRenderTarget::accum_direct_cascade(u32 cascade, Fmatrix& xform, Fmatrix& xform_prev, float bias)
{
    // init accumulator
    phase_accumulator();

    if (RImplementation.o.sunfilter)
    {
        accum_direct_f(cascade);
        return;
    }

    // sun source
    light* sun = (light*)RImplementation.Lights.sun_adapted._get();

    // screen quad params
    u32 offset = 0;
    u32 color = color_rgba(255, 255, 255, 255);

    float w = float(Device.dwWidth);
    float h = float(Device.dwHeight);

    Fvector2 uv0, uv1;
    uv0.set(.5f / w, .5f / h);
    uv1.set((w + .5f) / w, (h + .5f) / h);

    float depthZ = EPS_S;
    float depthW = 1.f;

    // light params
    Fvector L_dir, L_clr;
    float   L_spec = 0.f;

    L_clr.set(sun->color.r, sun->color.g, sun->color.b);
    L_spec = u_diffuse2s(L_clr);

    Device.mView.transform_dir(L_dir, sun->direction);
    L_dir.normalize();

    // mask (only near cascade)
    RCache.set_CullMode(CULL_NONE);

    if (cascade == SE_SUN_NEAR)
    {
        FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_combine->vb_stride, offset);

        pv->set(EPS, h + EPS, depthZ, depthW, color, uv0.x, uv1.y); pv++;
        pv->set(EPS, EPS,     depthZ, depthW, color, uv0.x, uv0.y); pv++;
        pv->set(w + EPS, h + EPS, depthZ, depthW, color, uv1.x, uv1.y); pv++;
        pv->set(w + EPS, EPS,     depthZ, depthW, color, uv1.x, uv0.y); pv++;

        RCache.Vertex.Unlock(4, g_combine->vb_stride);
        RCache.set_Geometry(g_combine);

        float intensity = 0.3f * L_clr.x + 0.48f * L_clr.y + 0.22f * L_clr.z;

        Fvector dir = L_dir;
        dir.mul(-_sqrt(intensity + EPS));

        RCache.set_Element(s_accum_mask->E[SE_MASK_DIRECT]);
        RCache.set_c("Ldynamic_dir", dir.x, dir.y, dir.z, 0);

        RCache.set_ColorWriteEnable(FALSE);
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff,
                           D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);

        RCache.Render(D3DPT_TRIANGLELIST, offset, 0, 4, 0, 2);
    }

    // recalc depth
    Fvector center;
    center.mad(Device.vCameraPosition, Device.vCameraDirection, ps_r2_sun_near);

    Device.mFullTransform.transform(center);
    depthZ = center.z;

    // stencil optimization
    if (RImplementation.o.nvstencil && cascade == SE_SUN_NEAR)
        u_stencil_optimize();

    // lighting pass
    phase_accumulator();

    RCache.set_CullMode(CULL_CCW);
    RCache.set_ColorWriteEnable();

    float texelOffset = 0.5f / float(RImplementation.o.smapsize);
    float range = (cascade == SE_SUN_NEAR) ? ps_r2_sun_depth_near_scale : ps_r2_sun_depth_far_scale;

    Fmatrix texAdjust =
    {
        0.5f, 0.f, 0.f, 0.f,
        0.f, -0.5f, 0.f, 0.f,
        0.f, 0.f, range, 0.f,
        0.5f + texelOffset, 0.5f + texelOffset, bias, 1.f
    };

    // matrices
    Fmatrix invView;
    invView.invert(Device.mView);

    Fmatrix shadow;
    {
        Fmatrix proj;
        proj.mul(texAdjust, sun->X.D.combine);
        shadow.mul(proj, invView);

        if (cascade == SE_SUN_FAR && RImplementation.o.HW_smap)
        {
            Fvector shift;
            shift.mul(L_dir, ps_r2_sun_tsm_bias);

            Fmatrix shiftM;
            shiftM.translate(shift);

            shadow.mulB_44(shiftM);
        }
    }

    // clouds shadow
    Fmatrix cloudsShadow;
    {
        static float shift = 0.f;
        shift += 0.003f * Device.fTimeDelta;

        float windDir = g_pGamePersistent->Environment.CurrentEnv.wind_direction;

        Fvector up; up.setHP(windDir, 0);
        Fvector pos; pos.set(0, 0, 0);

        Fmatrix basis;
        basis.build_camera_dir(pos, sun->direction, up);

        cloudsShadow.mul(basis, invView);

        Fmatrix scale;
        scale.scale(0.002f, 0.002f, 1.f);
        cloudsShadow.mulA_44(scale);

        Fvector moveVec = up;
        moveVec.mul(shift);

        Fmatrix move;
        move.translate(moveVec);

        cloudsShadow.mulA_44(move);
    }

    // texgen
    Fmatrix texgen;
    texgen.identity();

    RCache.xforms.set_W(texgen);
    RCache.xforms.set_V(Device.mView);
    RCache.xforms.set_P(Device.mProject);

    u_compute_texgen_screen(texgen);

    // geometry
    u32 i_offset = 0;

    {
        u16* pib = RCache.Index.Lock(sizeof(facetable) / sizeof(u16), i_offset);
        CopyMemory(pib, &facetable, sizeof(facetable));
        RCache.Index.Unlock(sizeof(facetable) / sizeof(u16));

        u32 vCount = sizeof(corners) / sizeof(Fvector3);

        FVF::L* pv = (FVF::L*)RCache.Vertex.Lock(vCount, g_combine_cuboid.stride(), offset);

        Fmatrix invShadow;
        invShadow.invert((cascade == SE_SUN_FAR) ? xform_prev : xform);

        for (u32 i = 0; i < vCount; ++i)
        {
            Fvector3 v;
            invShadow.transform(v, corners[i]);
            pv->set(v, color);
            pv++;
        }

        RCache.Vertex.Unlock(vCount, g_combine_cuboid.stride());
    }

    RCache.set_Geometry(g_combine_cuboid);

    // shader setup
    RCache.set_Element(s_accum_direct_cascade->E[cascade]);

    RCache.set_c("m_texgen", texgen);
    RCache.set_c("Ldynamic_dir", L_dir.x, L_dir.y, L_dir.z, 0);
    RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
    RCache.set_c("m_shadow", shadow);
    RCache.set_c("m_sunmask", cloudsShadow);

    if (cascade == SE_SUN_FAR)
    {
        Fvector3 viewDir;
        viewDir.set(0, 0, 1);

        shadow.transform_dir(viewDir);

        Fvector4 proj;
        proj.set(viewDir.x, viewDir.y, 0, 0);
        proj.normalize();

        RCache.set_c("view_shadow_proj", proj);
    }

    // depth func
    if (cascade == SE_SUN_NEAR || cascade == SE_SUN_MIDDLE)
        HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
    else if (!ps_r2_ls_flags.test(R2FLAGEXT_SUN_ZCULLING))
        HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    else
        HW.pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);

    // stencil
    if (cascade == SE_SUN_NEAR || cascade == SE_SUN_MIDDLE)
    {
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0xFE,
                           D3DSTENCILOP_KEEP, D3DSTENCILOP_ZERO, D3DSTENCILOP_KEEP);
    }
    else
    {
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
    }

    // draw
    RCache.Render(D3DPT_TRIANGLELIST, offset, 0, 8, 0, 16);
}

void CRenderTarget::accum_direct_blend	()
{
	// blend-copy
	if (!RImplementation.o.fp16_blend)	{
		u_setrt						(rt_Accumulator,NULL,NULL,HW.pBaseZB);

		// Common calc for quad-rendering
		u32		Offset;
		u32		C					= color_rgba	(255,255,255,255);
		float	_w					= float			(Device.dwWidth);
		float	_h					= float			(Device.dwHeight);
		Fvector2					p0,p1;
		p0.set						(.5f/_w, .5f/_h);
		p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
		float	d_Z	= EPS_S, d_W = 1.f;

		// Fill vertex buffer
		FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
		pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, p0.x, p1.y);	pv++;
		pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, p0.x, p0.y);	pv++;
		pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, p1.x, p1.y);	pv++;
		pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, p1.x, p0.y);	pv++;
		RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
		RCache.set_Geometry			(g_combine_2UV);
		RCache.set_Element			(s_accum_mask->E[SE_MASK_ACCUM_2D]	);
		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2	);
	}
	dwLightMarkerID				+= 2;
}

void CRenderTarget::accum_direct_f		(u32 sub_phase)
{
	// Select target
	if (SE_SUN_LUMINANCE==sub_phase)	{
		accum_direct_lum	();
		return				;
	}
	phase_accumulator					();
	u_setrt								(rt_Generic_0,NULL,NULL,HW.pBaseZB);

	// *** assume accumulator setted up ***
	light*			fuckingsun			= (light*)RImplementation.Lights.sun_adapted._get()	;

	// Common calc for quad-rendering
	u32		Offset;
	u32		C					= color_rgba	(255,255,255,255);
	float	_w					= float			(Device.dwWidth);
	float	_h					= float			(Device.dwHeight);
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	float	d_Z	= EPS_S, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir,L_clr;	float L_spec;
	L_clr.set					(fuckingsun->color.r,fuckingsun->color.g,fuckingsun->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_dir	(L_dir,fuckingsun->direction);
	L_dir.normalize				();

	// Perform masking (only once - on the first/near phase)
	RCache.set_CullMode			(CULL_NONE	);
	if (SE_SUN_NEAR==sub_phase)	//.
	{
		// For sun-filter - clear to zero
		CHK_DX	(HW.pDevice->Clear	( 0L, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0L));

		// Fill vertex buffer
		FVF::TL* pv					= (FVF::TL*)	RCache.Vertex.Lock	(4,g_combine->vb_stride,Offset);
		pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y);	pv++;
		pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y);	pv++;
		pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y);	pv++;
		pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y);	pv++;
		RCache.Vertex.Unlock		(4,g_combine->vb_stride);
		RCache.set_Geometry			(g_combine);

		// setup
		float	intensity			= 0.3f*fuckingsun->color.r + 0.48f*fuckingsun->color.g + 0.22f*fuckingsun->color.b;
		Fvector	dir					= L_dir;
		dir.normalize().mul	(- _sqrt(intensity+EPS));
		RCache.set_Element			(s_accum_mask->E[SE_MASK_DIRECT]);		// masker
		RCache.set_c				("Ldynamic_dir",		dir.x,dir.y,dir.z,0		);

		// if (stencil>=1 && aref_pass)	stencil = light_id
		RCache.set_ColorWriteEnable	(FALSE		);
		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0x01,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection,ps_r2_sun_near);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// nv-stencil recompression
	if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?

	// Perform lighting
	{
		u_setrt								(rt_Generic_0,NULL,NULL,HW.pBaseZB);  // enshure RT setup
		RCache.set_CullMode					(CULL_NONE	);
		RCache.set_ColorWriteEnable			();

		// texture adjustment matrix
		float			fTexelOffs			= (.5f / float(RImplementation.o.smapsize));
		float			fRange				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_scale:ps_r2_sun_depth_far_scale;
		float			fBias				= (SE_SUN_NEAR==sub_phase)?ps_r2_sun_depth_near_bias:ps_r2_sun_depth_far_bias;
		Fmatrix			m_TexelAdjust		= 
		{
			0.5f,				0.0f,				0.0f,			0.0f,
			0.0f,				-0.5f,				0.0f,			0.0f,
			0.0f,				0.0f,				fRange,			0.0f,
			0.5f + fTexelOffs,	0.5f + fTexelOffs,	fBias,			1.0f
		};

		// compute xforms
		Fmatrix				m_shadow;
		{
			FPU::m64r		();
			Fmatrix			xf_invview;		xf_invview.invert	(Device.mView)	;
			Fmatrix			xf_project;		xf_project.mul		(m_TexelAdjust,fuckingsun->X.D.combine);
			m_shadow.mul	(xf_project,	xf_invview);

			// tsm-bias
			if (SE_SUN_FAR == sub_phase)
			{
				Fvector		bias;	bias.mul		(L_dir,ps_r2_sun_tsm_bias);
				Fmatrix		bias_t;	bias_t.translate(bias);
				m_shadow.mulB_44	(bias_t);
			}
			FPU::m24r		();
		}

		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= float(Device.dwWidth)	/ float(TEX_jitter);
		//float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		// Fill vertex buffer
		FVF::TL2uv* pv				= (FVF::TL2uv*) RCache.Vertex.Lock	(4,g_combine_2UV->vb_stride,Offset);
		pv->set						(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y, j0.x, j1.y);	pv++;
		pv->set						(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y, j0.x, j0.y);	pv++;
		pv->set						(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y, j1.x, j1.y);	pv++;
		pv->set						(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y, j1.x, j0.y);	pv++;
		RCache.Vertex.Unlock		(4,g_combine_2UV->vb_stride);
		RCache.set_Geometry			(g_combine_2UV);

		// setup
		RCache.set_Element			(s_accum_direct->E[sub_phase]);
		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);
		RCache.set_c				("m_shadow",			m_shadow						);

		// setup stencil
		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}
}

void CRenderTarget::accum_direct_lum	()
{
	// Select target
	phase_accumulator					();

	// *** assume accumulator setted up ***
	light*			fuckingsun			= (light*)RImplementation.Lights.sun_adapted._get()	;

	// Common calc for quad-rendering
	u32		Offset;
	// u32		C					= color_rgba	(255,255,255,255);
	float	_w					= float			(Device.dwWidth);
	float	_h					= float			(Device.dwHeight);
	Fvector2					p0,p1;
	p0.set						(.5f/_w, .5f/_h);
	p1.set						((_w+.5f)/_w, (_h+.5f)/_h );
	float	d_Z	= EPS_S;		//, d_W = 1.f;

	// Common constants (light-related)
	Fvector		L_dir,L_clr;	float L_spec;
	L_clr.set					(fuckingsun->color.r,fuckingsun->color.g,fuckingsun->color.b);
	L_spec						= u_diffuse2s	(L_clr);
	Device.mView.transform_dir	(L_dir,fuckingsun->direction);
	L_dir.normalize				();

	// recalculate d_Z, to perform depth-clipping
	Fvector	center_pt;			center_pt.mad	(Device.vCameraPosition,Device.vCameraDirection,ps_r2_sun_near);
	Device.mFullTransform.transform(center_pt)	;
	d_Z							= center_pt.z	;

	// nv-stencil recompression
	/*
	if (RImplementation.o.nvstencil  && (SE_SUN_NEAR==sub_phase))	u_stencil_optimize();	//. driver bug?
	*/

	// Perform lighting
		RCache.set_CullMode					(CULL_NONE	);
		RCache.set_ColorWriteEnable			();

		// Make jitter texture
		Fvector2					j0,j1;
		float	scale_X				= float(Device.dwWidth)	/ float(TEX_jitter);
//		float	scale_Y				= float(Device.dwHeight)/ float(TEX_jitter);
		float	offset				= (.5f / float(TEX_jitter));
		j0.set						(offset,offset);
		j1.set						(scale_X,scale_X).add(offset);

		struct v_aa	{
			Fvector4	p;
			Fvector2	uv0;
			Fvector2	uvJ;
			Fvector2	uv1;
			Fvector2	uv2;
			Fvector2	uv3;
			Fvector4	uv4;
			Fvector4	uv5;
		};
		float	smooth				= 0.6f;
		float	ddw					= smooth/_w;
		float	ddh					= smooth/_h;

		// Fill vertex buffer
		VERIFY	(sizeof(v_aa)==g_aa_AA->vb_stride);
		v_aa* pv					= (v_aa*) RCache.Vertex.Lock	(4,g_aa_AA->vb_stride,Offset);
		pv->p.set(EPS,			float(_h+EPS),	EPS,1.f); pv->uv0.set(p0.x, p1.y);pv->uvJ.set(j0.x, j1.y);pv->uv1.set(p0.x-ddw,p1.y-ddh);pv->uv2.set(p0.x+ddw,p1.y+ddh);pv->uv3.set(p0.x+ddw,p1.y-ddh);pv->uv4.set(p0.x-ddw,p1.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		pv->p.set(EPS,			EPS,			EPS,1.f); pv->uv0.set(p0.x, p0.y);pv->uvJ.set(j0.x, j0.y);pv->uv1.set(p0.x-ddw,p0.y-ddh);pv->uv2.set(p0.x+ddw,p0.y+ddh);pv->uv3.set(p0.x+ddw,p0.y-ddh);pv->uv4.set(p0.x-ddw,p0.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		pv->p.set(float(_w+EPS),float(_h+EPS),	EPS,1.f); pv->uv0.set(p1.x, p1.y);pv->uvJ.set(j1.x, j1.y);pv->uv1.set(p1.x-ddw,p1.y-ddh);pv->uv2.set(p1.x+ddw,p1.y+ddh);pv->uv3.set(p1.x+ddw,p1.y-ddh);pv->uv4.set(p1.x-ddw,p1.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		pv->p.set(float(_w+EPS),EPS,			EPS,1.f); pv->uv0.set(p1.x, p0.y);pv->uvJ.set(j1.x, j0.y);pv->uv1.set(p1.x-ddw,p0.y-ddh);pv->uv2.set(p1.x+ddw,p0.y+ddh);pv->uv3.set(p1.x+ddw,p0.y-ddh);pv->uv4.set(p1.x-ddw,p0.y+ddh,0,0);pv->uv5.set(0,0,0,0);pv++;
		RCache.Vertex.Unlock		(4,g_aa_AA->vb_stride);
		RCache.set_Geometry			(g_aa_AA);

		// setup
		RCache.set_Element			(s_accum_direct->E[SE_SUN_LUMINANCE]);
		RCache.set_c				("Ldynamic_dir",		L_dir.x,L_dir.y,L_dir.z,0		);
		RCache.set_c				("Ldynamic_color",		L_clr.x,L_clr.y,L_clr.z,L_spec	);

		// setup stencil
		RCache.set_Stencil			(TRUE,D3DCMP_LESSEQUAL,dwLightMarkerID,0xff,0x00);
		RCache.Render				(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
}
