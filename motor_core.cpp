#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <emscripten/bind.h>

using namespace emscripten;
using namespace std;

// JavaScript tarafından çağrılabilen harici fonksiyonlar
extern "C" {
    extern void JS_LogOutput(const char* message);
    extern void JS_AddHierarchyItem(const char* name, int id);
    extern void JS_Spawn3DObject(int id, float x, float y, float z, float r, float g, float b);
    extern void JS_UpdateObject(int id, float x, float y, float z);
}

// ==========================================================
// 1. Component Sistemi (Bileşenler)
// ==========================================================

struct Component {
    string type;
    virtual ~Component() = default;
    // JS Properties paneli için JSON formatında çıktı üretir.
    virtual string to_json_properties() const = 0;
};

struct TransformComponent : public Component {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    TransformComponent() { type = "Transform"; }

    string to_json_properties() const override {
        stringstream ss;
        ss << R"("x": ")" << x << R"(", "y": ")" << y << R"(", "z": ")" << z << R"(")";
        return ss.str();
    }
    void set_property(const string& name, const string& value) {
        float val = stof(value);
        if (name == "x") x = val;
        else if (name == "y") y = val;
        else if (name == "z") z = val;
    }
};

struct MeshRendererComponent : public Component {
    string shape = "Block";
    float r = 0.5f;
    float g = 0.5f;
    float b = 0.5f;
    bool isVisible = true;
    MeshRendererComponent() { type = "MeshRenderer"; }

    string to_json_properties() const override {
        stringstream ss;
        ss << R"("shape": ")" << shape << R"(", "r": ")" << r << R"(", "g": ")" << g << R"(", "b": ")" << b << R"(", "isVisible": ")" << (isVisible ? "true" : "false") << R"(")";
        return ss.str();
    }
    void set_property(const string& name, const string& value) {
        if (name == "shape") shape = value;
        else if (name == "isVisible") isVisible = (value == "true");
        else if (name == "r" || name == "g" || name == "b") {
            float val = stof(value);
            if (name == "r") r = val;
            else if (name == "g") g = val;
            else if (name == "b") b = val;
        }
    }
};

// ==========================================================
// 2. Varlık Sistemi (Entity)
// ==========================================================

struct Entity {
    int id;
    string name;
    map<string, unique_ptr<Component>> components;

    Entity(int _id, const string& _name) : id(_id), name(_name) {}
    
    // Yardımcı fonksiyon: Component'e kolay erişim
    template<typename T>
    T* GetComponent(const string& typeName) {
        return static_cast<T*>(components[typeName].get());
    }
};

// ==========================================================
// 3. Oyun Dünyası ve Motor Çekirdeği (Singleton)
// ==========================================================

class LunXEngine {
private:
    map<int, unique_ptr<Entity>> entities;
    int nextEntityId = 1;
    LunXEngine() {}
    ~LunXEngine() = default;

public:
    static LunXEngine& Get() {
        static LunXEngine instance;
        return instance;
    }

    void CreatePart(const string& name) {
        int id = nextEntityId++;
        auto newEntity = make_unique<Entity>(id, name + to_string(id));
        
        // Temel Komponentleri Ekle
        newEntity->components["Transform"] = make_unique<TransformComponent>();
        newEntity->components["MeshRenderer"] = make_unique<MeshRendererComponent>();

        // Başlangıç Konumlarını Rastgele Ayarla
        TransformComponent* t = newEntity->GetComponent<TransformComponent>("Transform");
        t->x = (float)(rand() % 10) - 5;
        t->y = (float)(rand() % 5) + 1;
        t->z = (float)(rand() % 10) - 5;
        
        // Renkleri Rastgele Ayarla
        MeshRendererComponent* m = newEntity->GetComponent<MeshRendererComponent>("MeshRenderer");
        m->r = (float)rand() / RAND_MAX;
        m->g = (float)rand() / RAND_MAX;
        m->b = (float)rand() / RAND_MAX;
        
        // JS Arayüzünü Güncelle
        JS_AddHierarchyItem(newEntity->name.c_str(), id);
        JS_Spawn3DObject(id, t->x, t->y, t->z, m->r, m->g, m->b);

        string logMessage = "[GameWorld]: Yeni varlık oluşturuldu: " + newEntity->name;
        JS_LogOutput(logMessage.c_str());

        entities[id] = std::move(newEntity);
    }
    
    // Tüm Komponent Özelliklerini JSON olarak döndürür
    string GetEntityProperties(int id) {
        if (!entities.count(id)) {
            return R"({"error": "Entity bulunamadı."})";
        }
        Entity* entity = entities[id].get();
        stringstream ss;
        ss << R"({"name": ")" << entity->name << R"(", "id": )" << id << R"(, "components": {)";

        bool firstComponent = true;
        for (const auto& pair : entity->components) {
            if (!firstComponent) ss << ",";
            ss << R"(")" << pair.first << R"(": {)" << pair.second->to_json_properties() << R"(})";
            firstComponent = false;
        }

        ss << R"(}})";
        return ss.str();
    }
    
    // JS'den gelen tek bir özellik güncellemesini işler
    void SetEntityProperty(int id, const string& componentType, const string& propName, const string& propValue) {
        if (!entities.count(id)) return;
        Entity* entity = entities[id].get();

        if (entity->components.count(componentType)) {
            Component* comp = entity->components[componentType].get();
            
            // Komponent tipine göre doğru set_property fonksiyonunu çağır
            if (componentType == "Transform") {
                TransformComponent* t = static_cast<TransformComponent*>(comp);
                t->set_property(propName, propValue);
                
                // Konum değiştiyse 3D görünümü güncelle
                JS_UpdateObject(id, t->x, t->y, t->z);
            } else if (componentType == "MeshRenderer") {
                MeshRendererComponent* m = static_cast<MeshRendererComponent*>(comp);
                m->set_property(propName, propValue);
            }
            
            string log = "[C++]: " + entity->name + " varlığının " + componentType + "." + propName + " özelliği " + propValue + " olarak güncellendi.";
            JS_LogOutput(log.c_str());
        }
    }
    
    // Luau betik sistemi için basit bir arayüz simülasyonu
    void ExecuteScript(const string& script) {
        JS_LogOutput("[Luau VM]: Betik çalıştırma simülasyonu başlatıldı.");

        if (script.find("CreatePart()") != string::npos) {
            CreatePart("ScriptedPart");
        } else if (script.find("RotateWorld()") != string::npos) {
             // 3D sahneyi döndürme emri JS'e gönderilebilir
             JS_LogOutput("[Luau VM]: 'RotateWorld()' çağrısı simüle ediliyor. 3D sahne dönüyor...");
        } else {
            JS_LogOutput("[Luau VM]: Geçerli komut bulunamadı veya simüle edilmedi.");
        }
        
        JS_LogOutput("[Luau VM]: Çalıştırma tamamlandı.");
    }
};


// ==========================================================
// 4. Emscripten EXPORT Fonksiyonları
// ==========================================================

// Motorun başlatılması
void InitializeEngine() {
    JS_LogOutput("---------------------------------------");
    JS_LogOutput("LunX Studio Motoru BAŞLATILIYOR (C++ ECS)");
    JS_LogOutput("---------------------------------------");
    
    LunXEngine::Get().CreatePart("Baseplate");
    LunXEngine::Get().CreatePart("SpawnPoint");

    JS_LogOutput("Motor Başlatma Tamamlandı. Lütfen Hiyerarşiden bir varlık seçin.");
}

// UI Butonundan Yeni Varlık Oluşturma
void CreateNewBlockFromUI() {
    LunXEngine::Get().CreatePart("UISpawnedPart"); 
}

// Luau betiğini çalıştırma
void RunScript(const char* script) {
    if (script) {
        LunXEngine::Get().ExecuteScript(string(script));
    }
}

// Seçilen Varlığın Özelliklerini Almak
char* GetEntityProperties(int id) {
    string jsonString = LunXEngine::Get().GetEntityProperties(id);
    // JSON dizesini JS'ye geri göndermek için dinamik olarak ayır
    char* result = (char*)malloc(jsonString.length() + 1);
    strcpy(result, jsonString.c_str());
    return result;
}

// Varlık Özelliğini Güncellemek
void SetEntityProperty(int id, const char* compType, const char* propName, const char* propValue) {
    LunXEngine::Get().SetEntityProperty(id, string(compType), string(propName), string(propValue));
}


// ==========================================================
// Emscripten Bağlantı Noktaları (Gereklidir)
// ==========================================================

EMSCRIPTEN_BINDINGS(lunx_module) {
    function("InitializeEngine", &InitializeEngine);
    function("RunScript", &RunScript);
    function("CreateNewBlockFromUI", &CreateNewBlockFromUI);
    function("GetEntityProperties", &GetEntityProperties, allow_raw_pointer<char>());
    function("SetEntityProperty", &SetEntityProperty);
}
