#pragma once

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #ifdef SIMPLECOMMKIT_SHARED
        #ifdef SIMPLECOMMKIT_EXPORTS
            #define SIMPLECOMMKIT_API __declspec(dllexport)
        #else
            #define SIMPLECOMMKIT_API __declspec(dllimport)
        #endif
    #else
        #define SIMPLECOMMKIT_API
    #endif
#else
    #if __GNUC__ >= 4
        #ifdef SIMPLECOMMKIT_SHARED
            #ifdef SIMPLECOMMKIT_EXPORTS
                #define SIMPLECOMMKIT_API __attribute__((visibility("default")))
            #else
                #define SIMPLECOMMKIT_API
            #endif
        #else
            #define SIMPLECOMMKIT_API
        #endif
    #else
        #define SIMPLECOMMKIT_API
    #endif
#endif

#define SIMPLECOMMKIT_DECLARE_PRIVATE_CLASS(Class) \
class Class##Private;

#define SIMPLECOMMKIT_DECLARE_PRIVATE(Class) \
inline Class##Private* d_func() { \
        return reinterpret_cast<Class##Private*>(d_ptr.get()); \
} \
    inline const Class##Private* d_func() const { \
        return reinterpret_cast<const Class##Private*>(d_ptr.get()); \
} \
    friend class Class##Private;

// 私有类使用：生成 q_func()
#define SIMPLECOMMKIT_DECLARE_PUBLIC(Class) \
inline Class* q_func() { \
        return static_cast<Class*>(q_ptr); \
} \
    inline const Class* q_func() const { \
        return static_cast<const Class*>(q_ptr); \
} \
    friend class Class;

// 快捷访问宏
#define SIMPLECOMMKIT_D(Class) Class##Private* d = d_func();
#define SIMPLECOMMKIT_Q(Class) Class* q = q_func();
