#include "precompiled.h"

#include "Game.h"
#include "CLogger.h"
#ifndef NO_GUI
#include "gui/CGUI.h"
#endif

CGame *g_Game=NULL;

namespace PlayerArray_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CGameAttributes *pInstance=(CGameAttributes *)JS_GetPrivate(cx, obj);
		if (!JSVAL_IS_INT(id))
			return JS_FALSE;
		uint index=g_ScriptingHost.ValueToInt(id);
		uint numPlayers=pInstance->GetValue("numPlayers").ToUInt();

		if (numPlayers > pInstance->m_PlayerAttribs.size())
		{
			for (size_t i=pInstance->m_PlayerAttribs.size();i<=index;i++)
			{
				pInstance->m_PlayerAttribs.push_back(new CGameAttributes::CPlayerAttributes());
			}
		}

		if (index > numPlayers)
			return JS_FALSE;

		*vp=OBJECT_TO_JSVAL(pInstance->m_PlayerAttribs[index]->GetJSObject());
		return JS_TRUE;
	}

	JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		return JS_FALSE;
	}

	JSClass Class = {
		"PlayerArray", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSBool Construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, obj);
		*rval=OBJECT_TO_JSVAL(newObj);
		return JS_TRUE;
	}
};

namespace PlayerAttribs_JS
{
	void Finalize(JSContext *cx, JSObject *obj)
	{
		CAttributeMap *pAttribMap=(CAttributeMap *)JS_GetPrivate(cx, obj);
		delete pAttribMap;
	}

	JSClass Class = {
		"PlayerAttributes", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		AttributeMap_JS::GetProperty, AttributeMap_JS::SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, Finalize
	};

};

void CGameAttributes::CPlayerAttributes::CreateJSObject()
{
	ONCE(
		g_ScriptingHost.DefineCustomObjectType(&PlayerAttribs_JS::Class, AttributeMap_JS::Construct, 0, NULL, NULL, NULL, NULL);
	);
	m_JSObject=g_ScriptingHost.CreateCustomObject("PlayerAttributes");
	JS_SetPrivate(g_ScriptingHost.getContext(), m_JSObject, (CAttributeMap *)this);
	CAttributeMap::CreateJSObject();
}

CGameAttributes::CPlayerAttributes::CPlayerAttributes()
{
	AddValue("name", L"Player Default Name");
}

CGameAttributes::CGameAttributes():
	m_PlayerArrayJS(NULL)
{
	AddValue("mapFile", L"test01.pmp");
	AddValue("numPlayers", L"2");
}

CGameAttributes::~CGameAttributes()
{
	std::vector<CPlayerAttributes *>::iterator it=m_PlayerAttribs.begin();
	while (it != m_PlayerAttribs.end())
	{
		delete *it;
		++it;
	}
}

void CGameAttributes::CreateJSObject()
{
	CAttributeMap::CreateJSObject();

	ONCE(
		g_ScriptingHost.DefineCustomObjectType(&PlayerArray_JS::Class,
			PlayerArray_JS::Construct, 0, NULL, NULL, NULL, NULL);
	);

	m_PlayerArrayJS=g_ScriptingHost.CreateCustomObject("PlayerArray");
	JS_SetPrivate(g_ScriptingHost.GetContext(), m_PlayerArrayJS, this);
	int flags=JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT;
	JS_DefineProperty(g_ScriptingHost.GetContext(), m_JSObject, "players",
		OBJECT_TO_JSVAL(m_PlayerArrayJS), NULL, NULL, flags);
}

JSBool CGameAttributes::GetJSProperty(jsval id, jsval *ret)
{
	CStr name=g_ScriptingHost.ValueToString(id);
	if (name == CStr("players"))
		return JS_TRUE;
	return CAttributeMap::GetJSProperty(id, ret);
}

// Disable "warning C4355: 'this' : used in base member initializer list".
//   "The base-class constructors and class member constructors are called before
//   this constructor. In effect, you've passed a pointer to an unconstructed
//   object to another constructor. If those other constructors access any
//   members or call member functions on this, the result will be undefined."
// In this case, the pointers are simply stored for later use, so there
// should be no problem.
#ifdef _MSC_VER
# pragma warning (disable: 4355)
#endif

CGame::CGame():
	m_World(this),
	m_Simulation(this),
	m_GameView(this),
	m_pLocalPlayer(NULL),
	m_GameStarted(false)
{
	debug_out("CGame::CGame(): Game object CREATED\n");
}

#ifdef _MSC_VER
# pragma warning (default: 4355)
#endif

CGame::~CGame()
{
	for (size_t i=0; i<m_Players.size(); ++i)
		delete m_Players[i];

	debug_out("CGame::~CGame(): Game object DESTROYED\n");
}

PSRETURN CGame::StartGame(CGameAttributes *pAttribs)
{
	try
	{
		// Free old memory
		for (size_t i=0; i<m_Players.size(); ++i)
			delete m_Players[i];

		m_NumPlayers=pAttribs->GetValue("numPlayers").ToUInt();

		// Note: If m_Players is resized after this point (causing a reallocation)
		// various bits of code will still contain pointers to data at the original
		// locations. This is seldom a good thing. Make it big enough here.

		// Player 0 = Gaia

		m_Players.resize(m_NumPlayers + 1);

		for (uint i=0;i <= m_NumPlayers;i++)
			m_Players[i]=new CPlayer(i);
		
		// FIXME If the GUI hasn't set attributes for all players, the CPlayer
		// object's state will be whatever is set by the CPlayer constructor.
		for (uint i=0;i<m_NumPlayers && i<pAttribs->m_PlayerAttribs.size();i++)
		{
			CGameAttributes::CPlayerAttributes *pPlayerAttribs=
				pAttribs->m_PlayerAttribs[i];
			// TODO Set player attributes in the player object
		}
			
		m_Players[0]->m_Name = L"Gaia";
		m_Players[0]->m_Colour.r = 0.2f;
		m_Players[0]->m_Colour.g = 0.7f;
		m_Players[0]->m_Colour.b = 0.2f;

		m_Players[1]->m_Name = L"Acumen";
		m_Players[1]->m_Colour.r = 1.0f;
		m_Players[1]->m_Colour.g = 0.0f;
		m_Players[1]->m_Colour.b = 0.0f;

		m_Players[2]->m_Name = L"Boco the Insignificant";
		m_Players[2]->m_Colour.r = 0.0f;
		m_Players[2]->m_Colour.g = 0.0f;
		m_Players[2]->m_Colour.b = 1.0f;

		m_pLocalPlayer=m_Players[1];

		// RC, 040804 - GameView needs to be initialised before World, otherwise GameView initialisation
		// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
		// values.  At the minute, it's just lighting settings, but could be extended to store camera position.  
		// Storing lighting settings in the gameview seems a little odd, but it's no big deal; maybe move it at 
		// some point to be stored in the world object?
		m_GameView.Initialize(pAttribs);
		m_World.Initialize(pAttribs);
		m_Simulation.Initialize(pAttribs);

		m_GameStarted=true;

#ifndef NO_GUI
		g_GUI.SendEventToAll("sessionstart");
#endif
	}
	catch (PSERROR_Game e)
	{
		return e.code;
	}
	return 0;
}

void CGame::Update(double deltaTime)
{
	m_Simulation.Update(deltaTime);
	
	// TODO Detect game over and bring up the summary screen or something
}
