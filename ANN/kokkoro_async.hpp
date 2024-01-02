KOKKORO_BEGIN

template<typename r_arg, typename ... args> constexpr std::function<r_arg(args...)> function_capsulate(r_arg (*func)(args...)) { return static_cast<r_arg(*)(args...)>(func); }

template<typename f_arg, typename ... args> constexpr auto function_package(f_arg &&func, args &&...paras) { return std::make_shared<std::packaged_task<std::invoke_result_t<f_arg, args...>()>>(std::bind(std::forward<f_arg>(func), std::forward<args>(paras)...)); }

// #define arg int
template <typename arg>
class kokkoro_queue {
protected: struct kokkoro_node {
    arg elem {};
    
    kokkoro_node *next = nullptr,
                 *prev = next;
};

public:
    kokkoro_queue() = default;

    uint64_t size() {
        if (stop) return 0;
        std::unique_lock<std::mutex> lk {td_mtx};
        return len;
    }

    template<typename...args>
    void en_queue(args &&...params) {
        auto tmp  = new kokkoro_node;
        tmp->elem = arg {std::forward<args>(params)...};
        {
            std::unique_lock<std::mutex> lk {td_mtx};
            if (len++) {
                tail->next = tmp;
                tmp->prev  = tail;
                tail       = tail->next;
            } else {
                head = tmp;
                tail = head;
            }
        }
        cond.notify_one();
    }

    arg de_queue() {
        kokkoro_node *tmp = nullptr;
        {
            std::unique_lock<std::mutex> lk {td_mtx};
            if (!len) cond.wait(lk);
            if (stop) return {};
            tmp  = head;
            head = head->next;
            if (--len) head->prev = nullptr;
            else {
                head = nullptr;
                tail = head;
            }
        }
        tmp->next = nullptr;
        auto ret  = std::move(tmp->elem);
        while (tmp) {
            delete tmp;
            tmp = nullptr;
        }
        return ret;
    }
    
    void reset() {
        stop = true;
        cond.notify_all();
    }

    ~kokkoro_queue() {
        while (tail) {
            auto tmp  = tail->prev;
            if (tmp) {
                tmp->prev = nullptr;
                tmp->next = nullptr;
            }
            delete tail;
            tail = tmp;
            tmp  = nullptr;
        }
        head = tail;
        len  = 0;
    }
    
protected:
    std::mutex td_mtx;
    std::condition_variable cond;

    kokkoro_node *head = nullptr,
                 *tail = head;

    uint64_t len = 0;

    std::atomic_bool stop = false;
};

struct async_controller final {
public:
    async_controller() = default;

    async_controller(const async_controller &) {}

    void thread_sleep(uint64_t wait_ms = 0) {
        std::unique_lock<std::mutex> lk(td_mtx);
        if (wait_ms) cond.wait_for(lk, std::chrono::milliseconds(wait_ms));
        else cond.wait(lk);
    }

    void thread_wake_all() { cond.notify_all(); }

    void thread_wake_one() { cond.notify_one(); }

    async_controller &operator=(const async_controller &) {}

private:
    std::mutex td_mtx;
    std::condition_variable cond;
};

class async_pool final {
public:
    async_pool(uint64_t thread_size = kokkoro_async_core) :
        thd_set(thread_size) { for (auto i = 0ull; i < thread_size; ++i) thd_set[i] = std::thread([this] { while (true) {
            auto curr_tsk = tsk_que.de_queue();
            if (stop) return;
            curr_tsk();
        } }); }

    template<typename f_arg, typename ... args>
    auto add_task(f_arg &&func, args &&...params) {
        auto tsk_ptr = function_package(std::forward<f_arg>(func), std::forward<args>(params)...);
        auto tsk_ret = tsk_ptr->get_future();
        if (stop) throw std::runtime_error("Pool has been stopped.");
        tsk_que.en_queue([tsk_ptr]{ (*tsk_ptr)(); });
        return tsk_ret;
    }

    uint64_t size() const { return thd_set.length; }

    ~async_pool() {
        stop = true;
        tsk_que.reset();
        for (auto i = 0ull; i < thd_set.length; ++i) if (thd_set[i].joinable()) thd_set[i].join();
    }

private:
    kokkoro_set<std::thread> thd_set;

    kokkoro_queue<std::function<void()>> tsk_que;

    std::atomic_bool stop = false;
};

KOKKORO_END