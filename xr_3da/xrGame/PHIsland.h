#ifndef PH_ISLAND_H
#define PH_ISLAND_H

#pragma warning(disable:4995)
#pragma warning(disable:4267)
#include <../ode/src/objects.h>
#include <../ode/src/joint.h>
#pragma warning(default:4995)
#pragma warning(default:4267)

#include "PhysicsCommon.h"

//-----------------------------------------
// Flags for physics islands
//-----------------------------------------
class CPHIslandFlags
{
    static constexpr unsigned int base = 8;
    static constexpr unsigned int shift_to_variable = base / 2;
    static constexpr unsigned int mask_static = 0xf;

    Flags8 flags;

    enum EFlags : unsigned int
    {
        stActive = 1 << 0,
        flPrefereExactIntegration = 1 << 1
    };

public:
    CPHIslandFlags() { init(); }

    IC void init()
    {
        flags.zero();
        flags.set(stActive, TRUE);
        unmerge();
    }

    IC bool is_active() const
    {
        return flags.test(stActive << shift_to_variable);
    }

    IC void set_prefere_exact_integration()
    {
        flags.set(flPrefereExactIntegration, TRUE);
    }

    IC void unset_prefere_exact_integration()
    {
        flags.set(flPrefereExactIntegration, FALSE);
    }

    IC bool is_exact_integration_preferred() const
    {
        return flags.test(flPrefereExactIntegration << shift_to_variable);
    }

    IC void merge(CPHIslandFlags& aflags)
    {
        flags.flags |= (aflags.flags.flags & mask_static);
        aflags.flags.set(stActive << shift_to_variable, FALSE);
    }

    IC void unmerge()
    {
        flags.flags = ((flags.flags & mask_static) << shift_to_variable) | (flags.flags & mask_static);
    }
};

//-----------------------------------------
// Island class
//-----------------------------------------
class CPHIsland : public dxWorld
{
    CPHIslandFlags m_flags;
    dxBody* m_first_body{};
    dxJoint* m_first_joint{};
    dxJoint** m_joints_tail{};
    dxBody** m_bodies_tail{};
    CPHIsland* m_self_active{};

    int m_nj{};
    int m_nb{};

    static constexpr int JOINTS_LIMIT = 1500;
    static constexpr int BODIES_LIMIT = 500;

public:
    IC bool CheckSize() const
    {
        return nj < JOINTS_LIMIT && nb < BODIES_LIMIT;
    }

    IC int MaxJoints() const
    {
        return JOINTS_LIMIT - nj;
    }

    IC int MaxJoints(const CPHIsland* island) const
    {
        return MaxJoints() - island->nj;
    }

    IC int MaxBodies(const CPHIsland* island) const
    {
        return BODIES_LIMIT - nb - island->nb;
    }

    IC bool CanMerge(CPHIsland* island, int& MAX_JOINTS)
    {
        MAX_JOINTS = MaxJoints(island);
        return MAX_JOINTS > 0 && ((nb + island->nb) < BODIES_LIMIT);
    }

    IC bool IsActive() const { return m_flags.is_active(); }

    IC dWorldID DWorld() { return reinterpret_cast<dWorldID>(this); }

    IC dWorldID DActiveWorld() { return reinterpret_cast<dWorldID>(DActiveIsland()); }

    IC CPHIsland* DActiveIsland()
    {
        GoActive();
        return m_self_active;
    }

    IC void GoActive()
    {
        while (!m_self_active->m_flags.is_active())
            m_self_active = m_self_active->m_self_active;
    }

    IC void Merge(CPHIsland* island)
    {
        CPHIsland* first_island = DActiveIsland();
        CPHIsland* second_island = island->DActiveIsland();
        if (first_island == second_island) return;

        *(second_island->m_joints_tail) = first_island->firstjoint;
        first_island->firstjoint = second_island->firstjoint;
        if (first_island->nj == 0 && second_island->nj != 0)
        {
            first_island->m_joints_tail = second_island->m_joints_tail;
        }

        *(second_island->m_bodies_tail) = first_island->firstbody;
        first_island->firstbody = second_island->firstbody;

        first_island->nj += second_island->nj;
        first_island->nb += second_island->nb;

        VERIFY(!(*(first_island->m_bodies_tail)));
        VERIFY(!(*(first_island->m_joints_tail)));
        VERIFY(!((!(first_island->nj)) && (first_island->firstjoint)));

        second_island->m_self_active = first_island;
        m_flags.merge(second_island->m_flags);
    }

    IC void Unmerge()
    {
        firstjoint = m_first_joint;
        firstbody = m_first_body;
        if (!m_nj)
        {
            m_joints_tail = &firstjoint;
            *m_joints_tail = nullptr;
        }
        else
        {
            firstjoint->tome = reinterpret_cast<dObject**>(&firstjoint);
        }
        *m_joints_tail = nullptr;
        *m_bodies_tail = nullptr;

        m_flags.unmerge();
        m_self_active = this;
        nj = m_nj;
        nb = m_nb;
    }

    IC void Init()
    {
        m_flags.init();
        m_nj = nj = 0;
        m_nb = nb = 0;
        m_first_joint = firstjoint = nullptr;
        m_first_body = firstbody = nullptr;
        m_joints_tail = &firstjoint;
        m_bodies_tail = &firstbody;
        m_self_active = this;
    }

    IC void AddBody(dxBody* body)
    {
        VERIFY2(m_nj == nj && m_nb == nb && m_flags.is_active(), "cannot remove/add during processing phase");
        dWorldAddBody(DWorld(), body);
        m_first_body = body;
        if (m_nb == 0)
        {
            m_bodies_tail = reinterpret_cast<dxBody**>(&body->next);
        }
        m_nb++;
    }

    IC void RemoveBody(dxBody* body)
    {
        VERIFY2(m_nj == nj && m_nb == nb && m_flags.is_active(), "cannot remove/add during processing phase");
        if (m_first_body == body) m_first_body = reinterpret_cast<dxBody*>(body->next);
        if (m_bodies_tail == reinterpret_cast<dxBody**>(&(body->next)))
        {
            m_bodies_tail = reinterpret_cast<dxBody**>(body->tome);
        }
        dWorldRemoveBody(reinterpret_cast<dxWorld*>(this), body);
        m_nb--;
    }

    IC void AddJoint(dxJoint* joint)
    {
        VERIFY2(m_nj == nj && m_nb == nb && m_flags.is_active(), "cannot remove/add during processing phase");
        dWorldAddJoint(DWorld(), joint);
        m_first_joint = joint;
        if (!m_nj)
        {
            VERIFY(joint->next == nullptr);
            m_joints_tail = reinterpret_cast<dxJoint**>(&(joint->next));
        }
        m_nj++;
    }

    IC void ConnectJoint(dxJoint* joint)
    {
        if (!nj)
        {
            m_joints_tail = reinterpret_cast<dxJoint**>(&(joint->next));
            VERIFY(!firstjoint);
        }
        dWorldAddJoint(DWorld(), joint);
        VERIFY(!(*m_joints_tail));
    }

    IC void DisconnectJoint(dxJoint* joint)
    {
        dWorldRemoveJoint(DWorld(), joint);
    }

    IC void ConnectBody(dxBody* body)
    {
        dWorldAddBody(DWorld(), body);
    }

    IC void DisconnectBody(dxBody* body)
    {
        dWorldRemoveBody(DWorld(), body);
    }

    IC void RemoveJoint(dxJoint* joint)
    {
        VERIFY2(m_nj == nj && m_nb == nb && m_flags.is_active(), "cannot remove/add during processing phase");
        if (m_first_joint == joint)
            m_first_joint = reinterpret_cast<dxJoint*>(joint->next);
        if (m_joints_tail == reinterpret_cast<dxJoint**>(&(joint->next)))
        {
            m_joints_tail = reinterpret_cast<dxJoint**>(joint->tome);
        }
        dWorldRemoveJoint(DWorld(), joint);
        VERIFY(!(*m_joints_tail));
        m_nj--;
    }

    void SetPrefereExactIntegration() { m_flags.set_prefere_exact_integration(); }
    void Step(dReal step);
    void Enable();
    void Repair();
};

#endif // PH_ISLAND_H
