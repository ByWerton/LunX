//
// LunX Studio v3.0 - Yüksek Performanslı Varlık Bileşen Sistem (ECS) Motoru
// Mimar: Mehmet Bey'in Ticari Oyun Projeleri İçin Tasarlandı
// Hedef: Veri yerelliği, önbellek dostu işlem ve maksimum esneklik.
//

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <typeindex>
#include <algorithm>
#include <string>

// Emscripten Binding (JavaScript ile İletişim İçin Zorunlu)
#include <emscripten/bind.h>

using namespace emscripten;

// ===================================================================
// 1. Varlık (Entity) Tanımı
// Varlıklar, sadece benzersiz ID'lerden (EID) oluşur.
// ===================================================================
using EntityID = unsigned int;

class EntityManager {
private:
    std::vector<EntityID> availableIDs;
    EntityID nextID = 1;
    std::map<EntityID, std::map<std::type_index, std::shared_ptr<void>>> entityToComponents;

public:
    EntityID CreateEntity() {
        EntityID id = nextID++;
        // Gerçek motorda, ID'ler geri dönüştürülmeli ve versiyonlanmalıdır.
        entityToComponents[id] = {};
        std::cout << "[ECS] Yeni Varlık Oluşturuldu. ID: " << id << std::endl;
        
        // JS Arayüzüne bildirim (Mock)
        EM_ASM({
            const name = "Entity_" + $0;
            CppInterface.JS_AddHierarchyItem(allocate(intArrayFromString(name), 'i8', ALLOC_STACK), $0);
        }, id);
        
        return id;
    }

    void DestroyEntity(EntityID id) {
        if (entityToComponents.count(id)) {
            entityToComponents.erase(id);
            // availableIDs.push_back(id); // ID Geri Dönüşümü
            std::cout << "[ECS] Varlık Yok Edildi. ID: " << id << std::endl;
        }
    }

    // Gerçek motorda, bu sorgular (query) ile optimize edilir.
    const std::map<EntityID, std::map<std::type_index, std::shared_ptr<void>>>& GetAllEntities() const {
        return entityToComponents;
    }
    
    // Yüksek Seviye JS Fonksiyonu: Yeni bir Part oluşturur (Taşıma Componenti ile)
    void CreateNewBlockFromUI() {
        EntityID newID = CreateEntity();
        
        // Transform Component (Basit Başlangıç Pozisyonu)
        AddComponent<struct TransformComponent>(newID, {0.0f, 0.5f, 0.0f});
        
        // Renderable Component (Basit Mesh)
        AddComponent<struct RenderableComponent>(newID, {1.0f, 1.0f, 1.0f, 0xFF5733 /* Kırmızımsı Turuncu */});
        
        // Three.js'e Varlık Bilgisini Gönder
        EM_ASM({
            CppInterface.JS_Spawn3DObject($0, 0.0, 0.5, 0.0, 1.0, 0.34, 0.2);
        }, newID);
        
        std::cout << "[ECS] Blok Varlığı Sahneye Eklendi. ID: " << newID << std::endl;
    }

    // ===================================================================
    // 2. Bileşen (Component) Yönetimi
    // Component'ler sadece saf veridir (Data-Only).
    // ===================================================================

    template<typename T>
    void AddComponent(EntityID id, T component) {
        if (entityToComponents.count(id)) {
            // Type Erasure kullanarak bileşeni sakla (ECS'in kalbi)
            entityToComponents[id][std::type_index(typeid(T))] = std::make_shared<T>(component);
        }
    }

    template<typename T>
    T* GetComponent(EntityID id) {
        auto it = entityToComponents.find(id);
        if (it != entityToComponents.end()) {
            auto comp_it = it->second.find(std::type_index(typeid(T)));
            if (comp_it != it->second.end()) {
                // Shared_ptr'dan orijinal türüne güvenli cast
                return static_cast<T*>(comp_it->second.get());
            }
        }
        return nullptr;
    }
};

// ===================================================================
// 3. Bileşen (Component) Tanımları
// Gerçek motorda, bellekte bitişik (contiguous) depolanırlar (SoA - Structure of Arrays).
// ===================================================================

struct TransformComponent {
    float x, y, z;      // Pozisyon
    float rot_x, rot_y, rot_z; // Dönme (Quaternion tercih edilir)
    float scale_x, scale_y, scale_z; // Ölçek
    
    // Kapsayıcı Yapıcı
    TransformComponent(float _x, float _y, float _z) : x(_x), y(_y), z(_z), rot_x(0), rot_y(0), rot_z(0), scale_x(1), scale_y(1), scale_z(1) {}
    
    // JSON'a dönüştürme (JS'e gönderim için)
    std::string ToJson() const {
        return "{\"x\":" + std::to_string(x) +
               ", \"y\":" + std::to_string(y) +
               ", \"z\":" + std::to_string(z) +
               ", \"RotationX\":" + std::to_string(rot_x) +
               ", \"RotationY\":" + std::to_string(rot_y) +
               ", \"RotationZ\":" + std::to_string(rot_z) +
               ", \"ScaleX\":" + std::to_string(scale_x) +
               ", \"ScaleY\":" + std::to_string(scale_y) +
               ", \"ScaleZ\":" + std::to_string(scale_z) + "}";
    }
};

struct RenderableComponent {
    float r, g, b, a; // Renk (RGBA)
    unsigned int materialID; // Gerçek Materyal Kaynağı ID'si
    bool isVisible = true;
    
    RenderableComponent(float _r, float _g, float _b, unsigned int _mat) : r(_r), g(_g), b(_b), a(1.0f), materialID(_mat) {}

    // JSON'a dönüştürme (JS'e gönderim için)
    std::string ToJson() const {
        return "{\"R\":" + std::to_string(r) +
               ", \"G\":" + std::to_string(g) +
               ", \"B\":" + std::to_string(b) +
               ", \"MaterialID\":" + std::to_string(materialID) +
               ", \"Visible\":" + (isVisible ? "true" : "false") + "}";
    }
};

struct PhysicsComponent {
    float mass = 1.0f;
    float restitution = 0.5f; // Esneklik
    bool isStatic = false;
    // ... Daha karmaşık Fizik Verileri (Hız, İvme vb.)
};


// ===================================================================
// 4. Sistem (System) Tanımı
// Sistemler, belirli Component setlerine sahip Varlıklar üzerinde işlem yapar.
// ===================================================================

class MovementSystem {
public:
    // Gerçek motorda bu, her karede (frame) çağrılır.
    void Update(EntityManager& em, float deltaTime) {
        std::cout << "[SYSTEM] Hareket Sistemi Güncelleniyor..." << std::endl;
        
        // Yüksek seviyeli bir motor döngüsünde:
        // for (auto entity : em.Query<TransformComponent, PhysicsComponent>()) { ... }
        
        // Basit bir hareket simülasyonu (Mock):
        for (auto const& [id, components] : em.GetAllEntities()) {
            if (TransformComponent* transform = em.GetComponent<TransformComponent>(id)) {
                
                // Mock: Her varlığı hafifçe Y ekseninde yukarı hareket ettir.
                // transform->y += 0.005f * deltaTime; // Gerçek FPS'e bağlı hareket
                
                // Sadece göstermek için, pozisyonu sürekli X ekseninde zıplat.
                transform->x = std::sin(emscripten_get_now() / 1000.0f) * 0.1f;
                
                // JS'e güncelleme gönder
                EM_ASM({
                    CppInterface.JS_UpdateObject($0, $1, $2, $3);
                }, id, transform->x, transform->y, transform->z);
            }
        }
    }
};

// ===================================================================
// 5. Motor Yönetimi ve Ana Döngü
// ===================================================================

EntityManager g_entityManager;
MovementSystem g_movementSystem;

// Ana Motor Döngüsü (Emscripten tarafından her karede çağrılır)
void MainLoop() {
    float deltaTime = 1.0f / 60.0f; // Mock 60 FPS
    
    // 1. Sistemleri Çalıştır
    g_movementSystem.Update(g_entityManager, deltaTime);
    
    // 2. Render Sistemi Çalışır (THREE.js JS tarafında)
    // ...
}

// Emscripten/JS Arayüz Fonksiyonları

// Motor Başlatma (JS tarafından çağrılır)
void InitializeEngine() {
    std::cout << "-----------------------------------" << std::endl;
    std::cout << "LunX Studio ECS Motoru Başlatıldı." << std::endl;
    std::cout << "ECS Version: 3.0 (Data-Oriented)" << std::endl;
    std::cout << "-----------------------------------" << std::endl;
    
    // Temel Varlıkları Oluştur
    EntityID groundID = g_entityManager.CreateEntity();
    g_entityManager.AddComponent<TransformComponent>(groundID, {0.0f, 0.0f, 0.0f});
    g_entityManager.AddComponent<RenderableComponent>(groundID, {0.5f, 0.5f, 0.5f, 0x555555});
    
    // JS'e döngüyü başlatma talimatı
    emscripten_set_main_loop(MainLoop, 0, 1);
}

// JS Tarafından Çağrılan: Seçili Varlığın Özelliklerini Getir
char* GetEntityProperties(EntityID id) {
    std::string json = "{\"entityID\":" + std::to_string(id) + ", \"components\":{";
    
    TransformComponent* transform = g_entityManager.GetComponent<TransformComponent>(id);
    if (transform) {
        json += "\"Transform\":" + transform->ToJson() + ",";
    }
    
    RenderableComponent* renderable = g_entityManager.GetComponent<RenderableComponent>(id);
    if (renderable) {
        json += "\"Renderable\":" + renderable->ToJson() + ",";
    }
    
    // Son virgülü kaldır
    if (json.back() == ',') json.pop_back();
    
    json += "}}";
    
    // Emscripten_bind.h'den gelen string'i JS'e kopyalamak için heap'e ayır
    // Kopyalanan pointer'ın JS tarafında _free() ile serbest bırakılması GEREKLİDİR.
    char* cstr = new char[json.length() + 1];
    std::strcpy(cstr, json.c_str());
    return cstr;
}

// JS Tarafından Çağrılan: Varlık Özelliğini Güncelle
void SetEntityProperty(EntityID id, const char* compType, const char* propName, const char* propValue) {
    std::string compStr(compType);
    std::string propStr(propName);
    float value = std::stof(propValue); // Float/int için hızlı dönüşüm

    if (compStr == "Transform") {
        if (TransformComponent* transform = g_entityManager.GetComponent<TransformComponent>(id)) {
            if (propStr == "x") transform->x = value;
            else if (propStr == "y") transform->y = value;
            else if (propStr == "z") transform->z = value;
            else if (propStr == "RotationX") transform->rot_x = value;
            else if (propStr == "RotationY") transform->rot_y = value;
            else if (propStr == "RotationZ") transform->rot_z = value;
            else if (propStr == "ScaleX") transform->scale_x = value;
            else if (propStr == "ScaleY") transform->scale_y = value;
            else if (propStr == "ScaleZ") transform->scale_z = value;
            
            std::cout << "[ECS] Varlık " << id << " Transform." << propStr << " güncellendi: " << propValue << std::endl;
            
            // THREE.js'i anında güncelle
            EM_ASM({
                CppInterface.JS_UpdateObject($0, $1, $2, $3);
            }, id, transform->x, transform->y, transform->z);

            return;
        }
    } else if (compStr == "Renderable") {
        if (RenderableComponent* renderable = g_entityManager.GetComponent<RenderableComponent>(id)) {
             if (propStr == "R") renderable->r = value;
             else if (propStr == "G") renderable->g = value;
             else if (propStr == "B") renderable->b = value;
             else if (propStr == "Visible") renderable->isVisible = (std::string(propValue) == "true");

             std::cout << "[ECS] Varlık " << id << " Renderable." << propStr << " güncellendi: " << propValue << std::endl;
             return;
        }
    }

    std::cerr << "[ECS HATA] Özellik güncellenemedi: " << compType << "." << propName << std::endl;
}


// ===================================================================
// Emscripten Bindings (Gerekli)
// ===================================================================

EMSCRIPTEN_BINDINGS(engine_module) {
    function("InitializeEngine", &InitializeEngine);
    function("CreateNewBlockFromUI", &EntityManager::CreateNewBlockFromUI, &g_entityManager);
    function("GetEntityProperties", &GetEntityProperties, allow_raw_pointers());
    function("SetEntityProperty", &SetEntityProperty, allow_raw_pointers());
}
