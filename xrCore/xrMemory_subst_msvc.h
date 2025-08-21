#ifdef DEBUG
template <class T>
IC T* xr_new() {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T();
}
template <class T, class P1>
IC T* xr_new(const P1& p1) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1);
}
template <class T, class P1, class P2>
IC T* xr_new(const P1& p1, const P2& p2) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2);
}
template <class T, class P1, class P2, class P3>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3);
}
template <class T, class P1, class P2, class P3, class P4>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3, p4);
}
template <class T, class P1, class P2, class P3, class P4, class P5>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3, p4, p5);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3, p4, p5, p6);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3, p4, p5, p6, p7);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3, p4, p5, p6, p7, p8);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8, const P9& p9) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
    return new (ptr) T(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}
#else
template <class T>
IC T* xr_new() {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T();
}
template <class T, class P1>
IC T* xr_new(const P1& p1) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1);
}
template <class T, class P1, class P2>
IC T* xr_new(const P1& p1, const P2& p2) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2);
}
template <class T, class P1, class P2, class P3>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3);
}
template <class T, class P1, class P2, class P3, class P4>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3, p4);
}
template <class T, class P1, class P2, class P3, class P4, class P5>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3, p4, p5);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3, p4, p5, p6);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3, p4, p5, p6, p7);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3, p4, p5, p6, p7, p8);
}
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>
IC T* xr_new(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8, const P9& p9) {
    T* ptr = (T*)Memory.mem_alloc(sizeof(T));
    return new (ptr) T(p1, p2, p3, p4, p5, p6, p7, p8, p9);
}
#endif

template <bool _is_pm, typename T>
struct xr_special_free {
    IC void operator()(T*& ptr) {
        ptr->~T();
        Memory.mem_free((void*)ptr);
    }
};

template <typename T>
struct xr_special_free<false, T> {
    IC void operator()(T*& ptr) {
        ptr->~T();
        Memory.mem_free(ptr);
    }
};

template <class T>
IC void xr_delete(T*& ptr) {
    if (ptr) {
        xr_special_free<std::is_polymorphic<T>::value, T>()(ptr);
        ptr = NULL;
    }
}
template <class T>
IC void xr_delete(T* const& ptr) {
    if (ptr) {
        xr_special_free<std::is_polymorphic<T>::value, T>()(const_cast<T*&>(ptr));
        const_cast<T*&>(ptr) = NULL;
    }
}

#include <memory>

template<typename T>
using xr_weak_ptr = std::weak_ptr<T>;

template<typename T>
using xr_shared_ptr = std::shared_ptr<T>;

template<typename T>
using xr_unique_ptr = std::unique_ptr<T, xr_special_free<false, T>>;

template <class T, class... Args>
xr_shared_ptr<T> xr_make_shared(Args&&... args)
{
	return xr_shared_ptr<T>(new T(std::forward<Args>(args)...), [](T* ptr)
		{
			xr_special_free<false, T> deleter;
			deleter(ptr);
		});
}

template <typename T, typename... ARGS>
xr_unique_ptr<T> xr_make_unique(ARGS&&... args)
{
	void* TypeMem = Memory.mem_alloc(sizeof(T));
	new (TypeMem)T(std::forward<ARGS>(args)...);
	return xr_unique_ptr<T>(reinterpret_cast<T*>(TypeMem), xr_special_free<false, T>{});
}
