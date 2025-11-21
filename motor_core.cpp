#include <iostream>
#include <emscripten/emscripten.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <sstream>

// ==========================================================
// C++ MOTOR ÇEKİRDEĞİ (LunX Engine Core)
// Endüstriyel Oyun Motoru Mimarisi Simülasyonu
// ==========================================================

// --- 1. JS Köprü Fonksiyonları (Wasm -> JavaScript) ---
extern "C" {
    // Motor çıktılarını web konsoluna yönlendirir
    extern void JS_LogOutput(const char* message);
    // Hiyerarşi ağacına yeni bir obje eklenmesini bildirir
    extern void JS_AddHierarchyItem(const char* name, int id);
    // 3D görünümde bir objenin oluşturulması için komut gönderir
    extern void JS_Spawn3DObject(int id, float x, float y, float z);
}

// Luau Sanal Makinesi Simülasyonu (Roblox'un kullandığı Luau/Lua)
extern "C" {
    typedef struct lua_State lua_State;
    lua_State* luaL_newstate() { return (lua_State*)1; }
    void lua_settop(lua_State* L, int index) {}
    int lua_pcallk(lua_State* L, int nargs, int nresults, int errfunc, int k, int (*kfunc)(lua_State*)) { return 0; }
    void luaL_loadstring(lua_State* L, const char* s) {}
    void lua_close(lua_State* L) {}
}


// --- 2. ECS ve Varlık Yönetimi (Entity Component System) ---

using EntityID = unsigned int;

// TEMEL BİLEŞEN: Konum, bir objenin nerede olduğunu tanımlar.
struct PositionComponent {
    float x = 0, y = 0, z = 0;
};

// TEMEL BİLEŞEN: Görünüm, bir objenin nasıl göründüğünü tanımlar (Mesh, Texture)
struct VisualComponent {
    std::string meshPath = "DefaultCube.fbx";
    std::string texturePath = "Brick_Texture_001.png";
};

// Varlık Sınıfı (Basitçe bir Entity)
class Entity {
public:
    EntityID id;
    std::string name;
    
    PositionComponent position; 
    VisualComponent visual;

    Entity(EntityID _id, const std::string& _name) : id(_id), name(_name) {}
};


// --- 3. Motor Modülleri (Sistemler) ---

class GameWorld {
private:
    EntityID nextEntityID = 100;
    std::map<EntityID, std::shared_ptr<Entity>> entities;

public:
    void Initialize() {
        JS_LogOutput("[GameWorld]: Oyun Dünyası ve ECS kayıt sistemi başlatıldı.");
    }
    
    // Yeni bir Part (Blok) oluşturma
    std::shared_ptr<Entity> CreatePart(const std::string& name) {
        EntityID newID = nextEntityID++;
        std::stringstream ss;
        ss << name << "_" << newID;
        
        auto newEntity = std::make_shared<Entity>(newID, ss.str());
        
        // Başlangıç konumu (Yere düşmesin diye biraz havada)
        newEntity->position.y = 5.0f;
        
        entities[newID] = newEntity;
        
        // JS Arayüzüne Part'ın oluşturulduğunu ve Hiyerarşiye eklenmesi gerektiğini bildir
        JS_AddHierarchyItem(newEntity->name.c_str(), newID);
        JS_Spawn3DObject(newID, newEntity->position.x, newEntity->position.y, newEntity->position.z);
        
        JS_LogOutput("[GameWorld]: Yeni 'Part' (Blok) oluşturuldu: " + newEntity->name);
        return newEntity;
    }

    // Bir varlığın konumunu C++ içinden güncelleme
    void SetEntityPosition(EntityID id, float x, float y, float z) {
        if (entities.count(id)) {
            auto& pos = entities[id]->position;
            pos.x = x;
            pos.y = y;
            pos.z = z;
            JS_LogOutput("[GameWorld]: Varlık " + entities[id]->name + " konumu güncellendi.");
            // Gerçek motorda buradan 3D Rendering System'e güncelleme komutu gider.
        }
    }
};

class ScriptingSystem {
private:
    lua_State* L_luau = nullptr;
    GameWorld& world; // GameWorld'e referans

    // Luau'dan çağrılacak C++ fonksiyonu (Basit "print" simülasyonu)
    static int Luau_print(lua_State* L) {
        // Lua'dan gelen argümanları alıp JS konsoluna yönlendirir
        JS_LogOutput("[Luau VM Çıktı]: Hello from Lua/Luau!"); 
        return 0;
    }
    
public:
    ScriptingSystem(GameWorld& gw) : world(gw) {}

    void Initialize() {
        L_luau = luaL_newstate();
        if (L_luau) {
            JS_LogOutput("[ScriptingSystem]: Luau VM başlatıldı. C++ API'leri Luau'ya bağlanıyor.");
            // Örnek bir Luau-C++ bağlantısı (Normalde burada binlerce API çağrısı olur)
            // lua_pushcfunction(L_luau, Luau_print);
            // lua_setglobal(L_luau, "print");
        }
    }

    void RunScript(const char* luau_code) {
        JS_LogOutput("-> Betik Çalışıyor (Luau VM)...");
        // lua_pushstring(L_luau, luau_code);
        // luaL_loadstring(L_luau, luau_code);
        
        // Başarılı bir çağrı olduğunu simüle et
        if (std::string(luau_code).find("error") != std::string::npos) {
             JS_LogOutput("[ScriptingSystem HATA]: Betik Çalıştırılırken Sözdizimi Hatası! (Simülasyon)");
        } else {
             JS_LogOutput("-> Betik başarıyla tamamlandı.");
             // Eğer betik yeni bir Part oluşturuyorsa:
             if (std::string(luau_code).find("CreatePart") != std::string::npos) {
                 world.CreatePart("ScriptedPart");
             }
        }
    }
};

// --- 4. Ana Motor Sınıfı (Singleton) ---

class LunXEngine {
private:
    // SİSTEMLER
    // Binlerce satırlık motorun ana sistemleri burada yönetilir.
    // Physics, Rendering, Networking gibi sistemler de buraya eklenir.
    GameWorld gameWorld;
    std::unique_ptr<ScriptingSystem> scripting; // Scripting, GameWorld'e bağlıdır.

    LunXEngine() : scripting(std::make_unique<ScriptingSystem>(gameWorld)) {}
    LunXEngine(const LunXEngine&) = delete;
    void operator=(const LunXEngine&) = delete;

public:
    static LunXEngine& Get() {
        static LunXEngine instance;
        return instance;
    }

    // Motorun Ana Başlatma İşlemi
    void Initialize() {
        JS_LogOutput("\n--- LUNX PROFESSIONAL ENGINE ÇEKİRDEĞİ BAŞLATILIYOR ---\n");
        gameWorld.Initialize();
        scripting->Initialize();
        
        // Başlangıç sahneleri (Baseplate ve Spawn konumu gibi)
        gameWorld.CreatePart("Baseplate");
        gameWorld.CreatePart("SpawnPoint");
        
        JS_LogOutput("\n--- LUNX MOTOR ÇEKİRDEĞİ HAZIR. Lua/Luau Betiklerini Kabul Ediyor. ---\n");
    }

    // Luau Betiği Çalıştırma (JS'ten çağrılır)
    void ExecuteScript(const char* luau_code) {
        scripting->RunScript(luau_code);
    }
};

// ==========================================================
// Wasm/JavaScript Arayüzü (Emscripten Bağlantıları)
// ==========================================================

extern "C" {
    // Motoru başlatan fonksiyon (JS'ten çağrılır)
    EMSCRIPTEN_KEEPALIVE 
    void InitializeLunXStudio() {
        LunXEngine::Get().Initialize();
    }
    
    // Betik çalıştırma fonksiyonu (JS'ten çağrılır)
    EMSCRIPTEN_KEEPALIVE 
    void RunScript(const char* luau_code) {
        LunXEngine::Get().ExecuteScript(luau_code);
    }

    // JS Arayüzü: Yeni bir blok oluşturma butonu (JS'ten çağrılır)
    EMSCRIPTEN_KEEPALIVE
    void CreateNewBlockFromUI() {
        LunXEngine::Get().gameWorld.CreatePart("UISpawnedPart");
    }
}

int main() { 
    // Emscripten derlemesi için boş main
    return 0; 
}
