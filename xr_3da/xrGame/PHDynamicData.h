#pragma once

#include "ode_include.h"
#include "PHInterpolation.h"
#include "MathUtils.h"
#include <cstring>   // std::memcpy

class PHDynamicData
{
public:
    dVector3 pos{};
    dMatrix3 R{};
    Fmatrix BoneTransform{};

private:
    dBodyID body{};
    CPHInterpolation* p_parent_body_interpolation{};
    CPHInterpolation body_interpolation{};
    dGeomID geom{};
    dGeomID transform{};
    xr_vector<PHDynamicData> Childs;
    std::uint32_t numOfChilds{};
    Fmatrix ZeroTransform{};

public:
    inline void UpdateInterpolation()
    {
        body_interpolation.UpdatePositions();
        body_interpolation.UpdateRotations();
    }

    void UpdateInterpolationRecursive();
    void InterpolateTransform(Fmatrix& transform);
    void InterpolateTransformVsParent(Fmatrix& transform);

    PHDynamicData& operator[](std::uint32_t i) { return Childs[i]; }

    void Destroy();
    void Create(std::uint32_t numOfchilds, dBodyID Body);
    void CalculateData();
    PHDynamicData* GetChild(std::uint32_t ChildNum);
    bool SetChild(std::uint32_t ChildNum, std::uint32_t numOfchilds, dBodyID body);
    void SetAsZero();
    void SetAsZeroRecursive();
    void SetZeroTransform(Fmatrix& aTransform);

    PHDynamicData(std::uint32_t numOfchilds, dBodyID body);
    PHDynamicData();
    virtual ~PHDynamicData();

    void GetWorldMX(Fmatrix& aTransform)
    {
        dMatrix3 Rtmp{};
        dQtoR(dBodyGetQuaternion(body), Rtmp);
        DMXPStoFMX(Rtmp, dBodyGetPosition(body), aTransform);
    }

    void GetTGeomWorldMX(Fmatrix& aTransform)
    {
        if (!transform) return;

        Fmatrix NormTransform, TransformMx;
        dVector3 P0 = { 0, 0, 0, -1 };
        Fvector Translate{}, Translate1{};

        DMXPStoFMX(dBodyGetRotation(body), P0, NormTransform);
        DMXPStoFMX(dGeomGetRotation(dGeomTransformGetGeom(transform)), P0, TransformMx);

        dVectorSet((dReal*)&Translate, dGeomGetPosition(dGeomTransformGetGeom(transform)));
        dVectorSet((dReal*)&Translate1, dBodyGetPosition(body));

        aTransform.identity();
        aTransform.translate_over(Translate);
        aTransform.mulA_43(NormTransform);
        aTransform.translate_over(Translate1);
        aTransform.mulA_43(TransformMx);
    }

    inline static void DMXPStoFMX(const dReal* R, const dReal* pos, Fmatrix& aTransform)
    {
        std::memcpy(&aTransform, R, sizeof(dMatrix3));
        aTransform.transpose();
        std::memcpy(&aTransform.c, pos, sizeof(Fvector));
        aTransform._14 = 0.f;
        aTransform._24 = 0.f;
        aTransform._34 = 0.f;
        aTransform._44 = 1.f;
    }

    inline static void DMXtoFMX(const dReal* R, Fmatrix& aTransform)
    {
        aTransform._11 = R[0];  aTransform._12 = R[4];  aTransform._13 = R[8];  aTransform._14 = 0.f;
        aTransform._21 = R[1];  aTransform._22 = R[5];  aTransform._23 = R[9];  aTransform._24 = 0.f;
        aTransform._31 = R[2];  aTransform._32 = R[6];  aTransform._33 = R[10]; aTransform._34 = 0.f;
        aTransform._44 = 1.f;
    }

    inline static void FMX33toDMX(const Fmatrix33& aTransform, dReal* R)
    {
        R[0] = aTransform._11; R[4] = aTransform._12; R[8] = aTransform._13;
        R[1] = aTransform._21; R[5] = aTransform._22; R[9] = aTransform._23;
        R[2] = aTransform._31; R[6] = aTransform._32; R[10] = aTransform._33;
    }

    inline static void FMXtoDMX(const Fmatrix& aTransform, dReal* R)
    {
        R[0] = aTransform._11; R[4] = aTransform._12; R[8] = aTransform._13;
        R[1] = aTransform._21; R[5] = aTransform._22; R[9] = aTransform._23;
        R[2] = aTransform._31; R[6] = aTransform._32; R[10] = aTransform._33;
    }

private:
    void CalculateR_N_PosOfChilds(dBodyID parent);

public:
    bool SetGeom(dGeomID ageom);
    bool SetTransform(dGeomID ageom);
};
