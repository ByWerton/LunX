// Bu dosya, C++'tan çağrılan JavaScript fonksiyonlarını (extern "C" ile tanımlananları)
// Emscripten linker'ına tanıtır.
mergeInto(LibraryManager.library, {
    // C++ kodu bir string işaretçisi (messagePtr) gönderir.
    // JS tarafında bu işaretçiyi C-string'e çevirip kullanacağız.
    JS_LogOutput: function (messagePtr) {},
    
    // JS_AddHierarchyItem(const char* name, int id)
    JS_AddHierarchyItem: function (namePtr, id) {},

    // JS_Spawn3DObject(int id, float x, float y, float z, float r, float g, float b)
    JS_Spawn3DObject: function (id, x, y, z, r, g, b) {},

    // JS_UpdateObject(int id, float x, float y, float z)
    JS_UpdateObject: function (id, x, y, z) {},
});
