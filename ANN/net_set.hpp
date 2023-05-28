NEUNET_BEGIN

template <class arg>
class net_set {
protected:
    static void net_srand() { std::srand(std::time(NULL)); }

    void value_copy(const net_set &src) {
        if (this == &src) return;
        if (src.len != len) {
            reset();
            if (!src.len) return;
            init(src.len);
            len = src.len;
        }
        std::copy(src.ptr, src.ptr + len, ptr);
    }

    void value_move(net_set &&src) {
        if (this == &src) return;
        if (ptr) reset();
        len     = src.len;
        ptr     = src.ptr;
        src.ptr = nullptr;
        src.reset();
    }

public:
    net_set(uint64_t alloc_size = 0) : len(alloc_size) { if (len) { ptr = new arg[len](arg {}); } }
    net_set(std::initializer_list<arg> init_list) { if (init_list.size()) {
        ptr = new arg[init_list.size()];
        for (auto tmp : init_list) ptr[len++] = std::move(tmp);
    } }
    net_set(const net_set &src) { value_copy(src); }
    net_set(net_set &&src) { value_move(std::move(src)); }

    void init(uint64_t alloc_size, bool remain = true) {
        if (len == alloc_size) return;
        if (!alloc_size) {
            reset();
            return;
        }
        auto tmp = new double [alloc_size];
        if (remain) std::copy(ptr, ptr + std::min(len, alloc_size), tmp);
        reset();
        ptr = tmp;
        len = alloc_size;
        tmp = nullptr;
    }

    uint64_t size() const { return len; }

    void shuffle() {
        if (!len) return;
        for (auto i = len; i > 0; --i) std::swap(ptr[i - 1], ptr[std::rand() % i]);
    }

    void reverse() {
        if (len == 1) return;
        if (len == 2) {
            std::swap(ptr[0], ptr[1]);
            return;
        }
        std::reverse(ptr, ptr + len);    
    }

    void clear() {
        if (!len) return;
        if constexpr (std::is_arithmetic_v<arg>) std::memset(ptr, 0, len * sizeof(arg));
        else for (auto i = 0ull; i < len; ++i) ptr[i] = arg {};
    }

    void reset() {
        while (ptr) {
            delete [] ptr;
            ptr = nullptr;
        }
        len = 0;
    }
    
    ~net_set() { reset(); }

protected:
    arg *ptr = nullptr;

    uint64_t len = 0;
    
public:
    __declspec(property(get = size)) uint64_t length;

    net_set &operator=(const net_set &src) {
        value_copy(src);
        return *this;
    }
    net_set &operator=(net_set &&src) {
        value_move(std::move(src));
        return *this;
    }

    bool operator==(const net_set &src) const {
        if (this == &src) return true;
        if (src.len != len) return false;
        return std::equal(ptr, ptr + len, src.ptr);
    }

    arg &operator[](uint64_t idx) const {
        if (idx < len) return ptr[idx];
        return neunet_null_ref(arg);
    }
    
    friend std::ostream &operator<<(std::ostream &out, const net_set &src) {
        out << "[Length " << src.len << "]\n";
        for (auto i = 0ull; i < src.len; ++i) {
            out << '[' << i << "][\n";
            out << src.ptr[i] << "\n]";
            if (i + 1 != src.len) out << '\n';
        }
        return out;
    }

};

NEUNET_END