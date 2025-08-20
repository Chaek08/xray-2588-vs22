#include "stdafx.h"
#include "igame_level.h"
#include "feel_touch.h"
#include "xr_object.h"
using namespace Feel;

Touch::Touch():pure_relcase(&Touch::feel_touch_relcase)
{
}

Touch::~Touch()
{
}

BOOL Touch::feel_touch_contact	(CObject* O)
{ 
	return TRUE; 
}

void Touch::feel_touch_deny		(CObject* O, DWORD T)
{
	DenyTouch						D;
	D.O								= O;
	D.Expire						= Device.dwTimeGlobal + T;
	feel_touch_disable.push_back	(D);
}

void Touch::feel_touch_update(Fvector& C, float R)
{
    DWORD dwT = Device.dwTimeGlobal;

    // Удаляем истёкшие disable-объекты
    feel_touch_disable.erase(
        std::remove_if(feel_touch_disable.begin(), feel_touch_disable.end(),
            [dwT](const auto& d) { return d.Expire < dwT; }),
        feel_touch_disable.end()
    );

    // Находим ближайшие объекты
    xr_vector<CObject*> q_nearest;
    q_nearest.reserve(feel_touch.size());
    g_pGameLevel->ObjectSpace.GetNearest(q_nearest, C, R, nullptr);

    // Добавляем новые
    for (CObject* O : q_nearest) {
        if (O->getDestroy()) continue;
        if (!feel_touch_contact(O)) continue;

        if (std::find(feel_touch.begin(), feel_touch.end(), O) == feel_touch.end()) {
            // Проверка deny-листа
            bool bDeny = std::any_of(feel_touch_disable.begin(), feel_touch_disable.end(),
                [O](const auto& d) { return d.O == O; });

            if (!bDeny) {
                feel_touch.push_back(O);
                feel_touch_new(O);
            }
        }
    }

    // Удаляем старые
    feel_touch.erase(
        std::remove_if(feel_touch.begin(), feel_touch.end(),
            [&](CObject* O) {
                if (O->getDestroy() || !feel_touch_contact(O) ||
                    std::find(q_nearest.begin(), q_nearest.end(), O) == q_nearest.end())
                {
                    feel_touch_delete(O);
                    return true; // удалить
                }
                return false;
            }),
        feel_touch.end()
    );
}

void Touch::feel_touch_relcase	(CObject* O)
{
	xr_vector<CObject*>::iterator I = std::find (feel_touch.begin(),feel_touch.end(),O);
	if (I!=feel_touch.end()){
		feel_touch.erase		(I);
		feel_touch_delete		(O);
		}
	xr_vector<DenyTouch>::iterator Id=feel_touch_disable.begin(),IdE=feel_touch_disable.end();
	for(;Id!=IdE;++Id)			if((*Id).O==O )	{ feel_touch_disable.erase(Id); break; }
}
