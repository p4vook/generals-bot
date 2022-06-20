#pragma GCC optimize("Ofast,no-stack-protector")

#include <bits/stdc++.h>

struct PlayerInfo {
        unsigned army;
        unsigned land;
};

struct CellArmy {
        unsigned owner;
        unsigned size;
};

enum class ArmyType { Field, City, Capital };

enum class Hidden { Empty, Obstacle };

struct Mountain {
};

struct Armed {
        ArmyType type;
        CellArmy army;
};

using Visible = std::variant<Mountain, Armed>;
using Cell    = std::variant<Hidden, Visible>;

using CellI = std::pair<unsigned, unsigned>;

template <> struct std::hash<CellI> {
        std::size_t operator()(CellI const &x) const noexcept
        {
                return std::hash<unsigned>{}(x.first) ^
                    (std::hash<unsigned>{}(x.second) << 1);
        };
};

struct Skip {
};

enum class MoveType { All = 1, Half = 2 };

struct Move {
        MoveType type;
        CellI src;
        CellI dest;
};

using Turn = std::variant<Skip, Move>;

std::istream &operator>>(std::istream &in, PlayerInfo &x)
{
        return (in >> x.army >> x.land);
};

std::istream &operator>>(std::istream &in, Hidden &x)
{
        int t;
        in >> t;
        if (t == 1) {
                x = Hidden::Empty;
        } else if (t == 2) {
                x = Hidden::Obstacle;
        } else {
                throw std::runtime_error("Invalid type for hidden cell");
        }
        return in;
}

std::istream &operator>>(std::istream &in, CellArmy &x)
{
        in >> x.owner >> x.size;
        return in;
}

std::istream &operator>>(std::istream &in, Visible &x)
{
        int t;
        in >> t;
        if (t == 1 || t == 2 || t == 3) {
                Armed a;
                if (t == 1) {
                        a.type = ArmyType::Field;
                } else if (t == 2) {
                        a.type = ArmyType::City;
                } else if (t == 3) {
                        a.type = ArmyType::Capital;
                }
                in >> a.army;
                x = a;
        } else if (t == 4) {
                x = Mountain{};
        } else {
                throw std::runtime_error("Invalid type for visible cell");
        }
        return in;
}

std::istream &operator>>(std::istream &in, Cell &c)
{
        int visible;
        in >> visible;
        if (visible) {
                Visible v;
                in >> v;
                c = v;
        } else {
                Hidden h;
                in >> h;
                c = h;
        }
        return in;
}

std::ostream &operator<<(std::ostream &out, const Skip &)
{
        return (out << -1);
}

std::ostream &operator<<(std::ostream &out, const CellI &x)
{
        return (out << (x.second + 1) << " " << (x.first + 1));
}

std::ostream &operator<<(std::ostream &out, const Move &x)
{
        return (out << (int)x.type << " " << x.src << " " << x.dest);
}

std::ostream &operator<<(std::ostream &out, const Turn &x)
{
        if (std::holds_alternative<Skip>(x)) {
                out << std::get<Skip>(x) << std::endl;
        } else if (std::holds_alternative<Move>(x)) {
                out << std::get<Move>(x) << std::endl;
        } else {
                throw std::invalid_argument("Invalid turn");
        }
        return out;
}

struct Field {
        unsigned size_x;
        unsigned size_y;

        std::vector<std::vector<Cell>> table;

        Field(unsigned x, unsigned y) : size_x(x), size_y(y)
        {
                table.resize(x, std::vector<Cell>(y));
        }

        void read_table(std::istream &in)
        {
                for (unsigned y = 0; y < size_y; ++y) {
                        for (unsigned x = 0; x < size_x; ++x) {
                                in >> table[x][y];
                        }
                }
        }

        void check(const CellI &pos) const
        {
                if (pos.first < 0 || pos.first >= size_x) {
                        throw std::out_of_range("Field pos out of range\n");
                }
                if (pos.second < 0 || pos.second >= size_y) {
                        throw std::out_of_range("Field pos out of range\n");
                }
        }

        Cell at(const CellI &pos) const
        {
                check(pos);
                return table[pos.first][pos.second];
        }

        Cell &at(const CellI &pos)
        {
                check(pos);
                return table[pos.first][pos.second];
        };

        std::vector<CellI> neighbors(const CellI &pos) const
        {
                std::vector<CellI> res;
                if (pos.first > 0) {
                        res.push_back({pos.first - 1, pos.second});
                }
                if (pos.first + 1 < size_x) {
                        res.push_back({pos.first + 1, pos.second});
                }
                if (pos.second > 0) {
                        res.push_back({pos.first, pos.second - 1});
                }
                if (pos.second + 1 < size_y) {
                        res.push_back({pos.first, pos.second + 1});
                }
                return res;
        }

        unsigned dist(const CellI &pos1, const CellI &pos2) const {
                return std::abs((int) pos1.first - (int) pos2.first) + std::abs((int) pos1.second - (int) pos2.second);
        }
};

struct State {
        unsigned player_count;
        unsigned player_id;

        Field field;
        std::vector<PlayerInfo> info;

        unsigned turn_num;

        std::unordered_map<unsigned, std::pair<CellI, unsigned>> capital;
        std::deque<CellI> greedy_path;

        std::mt19937 rnd;

        State(unsigned x, unsigned y, unsigned _count, unsigned _id)
            : player_count(_count), player_id(_id), field(x, y),
              info(_count + 1), turn_num(0)
        {
                if (player_id > player_count) {
                        throw std::runtime_error("Invalid player id");
                }
        }

        void update_capitals()
        {
                for (unsigned x = 0; x < field.size_x; ++x) {
                        for (unsigned y = 0; y < field.size_y; ++y) {
                                auto &cell = field.at({x, y});
                                if (!std::holds_alternative<Visible>(cell)) {
                                        continue;
                                }
                                const auto &vis = std::get<Visible>(cell);
                                if (!std::holds_alternative<Armed>(vis)) {
                                        continue;
                                }
                                const auto &armed = std::get<Armed>(vis);
                                if (armed.type == ArmyType::Capital) {
                                        capital[armed.army.owner] = {
                                            {x, y}, armed.army.size};
                                }
                        }
                }
        }

        void read_next(std::istream &in)
        {
                ++turn_num;
                for (unsigned i = 1; i <= player_count; ++i) {
                        in >> info[i];
                }

                field.read_table(in);
                update_capitals();
        }

        struct PathRes {
                unsigned size;
                unsigned dist_sum;

                bool operator<(const PathRes &other) const {
                        return size > other.size || (size == other.size && dist_sum < other.dist_sum);
                }
        };

        PathRes count_path_res(const std::deque<CellI> &x) const {
                auto dist = [&](const CellI &pos) -> unsigned {
                        return field.dist(pos, capital.find(player_id)->second.first);
                };

                auto filter = [&](const CellI &pos) -> bool {
                        const auto &c = field.at(pos);
                        if (std::holds_alternative<Hidden>(c)) {
                                return true;
                        }
                        if (std::holds_alternative<Visible>(c)) {
                                const auto &v = std::get<Visible>(c);
                                return (std::holds_alternative<Armed>(v) &&
                                        std::get<Armed>(v).army.owner !=
                                            player_id);
                        }
                        return false;
                };

                unsigned size_x = 0;
                unsigned sum_dist = 0;

                for (const auto &tmp : x) {
                        size_x += filter(tmp);
                        sum_dist += dist(tmp);
                }

                return {size_x, sum_dist};
        }

        std::optional<int> capture_cost(const CellI &pos) const {
                const auto &c = field.at(pos);
                if (std::holds_alternative<Visible>(c)) {
                        const auto &V = std::get<Visible>(c);
                        if (std::holds_alternative<Armed>(V)) {
                                const auto &A = std::get<Armed>(V);
                                if (A.army.owner == player_id) {
                                        return 1 - A.army.size;
                                } else {
                                        return A.army.size + 1;
                                }
                        } else {
                                return std::nullopt;
                        }
                } else {
                        const auto &H = std::get<Hidden>(c);
                        if (H == Hidden::Obstacle) {
                                return std::nullopt;
                        } else if (H == Hidden::Empty) {
                                return 1;
                        }
                }
                return std::nullopt;
        }

        struct PathGeneratorState {
                std::mt19937 rnd;
                std::deque<CellI> cur;
                std::unordered_set<CellI> used;
                unsigned units;
                unsigned iterations = 0;
        };

        template <class ExitCondition, class ComparePaths>
        std::deque<CellI> gen_path(PathGeneratorState &state, ExitCondition &exit, ComparePaths &comp) const
        {
                ++state.iterations;
                std::deque<CellI> res = state.cur;
                if (state.units == 0 || exit(state)) {
                        return res;
                }

                state.used.insert(state.cur.back());
                auto neighbors = field.neighbors(state.cur.back());
                std::shuffle(neighbors.begin(), neighbors.end(), state.rnd);
                for (const auto &cand : neighbors) {
                        auto c = capture_cost(cand);
                        if (c && state.units > (unsigned) std::max(0, *c) && state.used.find(cand) == state.used.end() && c) {
                                state.cur.push_back(cand);
                                state.units -= *c;
                                auto next_res = gen_path(state, exit, comp);
                                if (comp(next_res, res)) {
                                        res = std::move(next_res);
                                }
                                state.units += *c;
                                state.cur.pop_back();
                        }
                }
                state.used.erase(state.cur.back());
                return res;
        }

        unsigned my_units(const CellI &pos) const {
                const auto &c = field.at(pos);
                if (!std::holds_alternative<Visible>(c)) {
                        return 0;
                }
                const auto &v = std::get<Visible>(c);
                if (!std::holds_alternative<Armed>(v)) {
                        return 0;
                }
                const auto &a = std::get<Armed>(v);
                if (a.army.owner == player_id) {
                        return a.army.size;
                } else {
                        return 0;
                }
        }

        Turn greedy_start()
        {
                if (greedy_path.size() < 2 && capital[player_id].second <= 10) {
                        return Skip{};
                } else {
                        auto cmp_res = [&](const std::deque<CellI> &x, const std::deque<CellI> &y) -> bool {
                                return count_path_res(x) < count_path_res(y);
                        };
                        auto exit_condition = [](const PathGeneratorState &s) -> bool {
                                return s.cur.size() >= 10 || s.iterations > 5000;
                        };

                        CellI begin;
                        std::deque<CellI> prev_path;
                        if (greedy_path.empty() || my_units(greedy_path.front()) <= 1) {
                                begin = capital[player_id].first;
                        } else {
                                begin = greedy_path.front();

                                {
                                        PathGeneratorState s;
                                        s.cur = std::move(greedy_path);
                                        for (const auto &CellI : s.cur) {
                                                s.used.insert(CellI);
                                        }
                                        s.iterations = 0;
                                        s.units      = my_units(s.cur.back()) - 1;
                                        s.rnd.seed(std::random_device{}());
                                        prev_path = gen_path(s, exit_condition, cmp_res);
                                }
                        }

                        {
                                PathGeneratorState s;
                                s.cur = {begin};
                                s.iterations = 0;
                                s.units = my_units(s.cur.back()) - 1;
                                s.rnd.seed(std::random_device{}());
                                greedy_path = gen_path(s, exit_condition, cmp_res);
                        }

                        if (count_path_res(prev_path) < count_path_res(greedy_path)) {
                                greedy_path = std::move(prev_path);
                        }
                        if (greedy_path.size() < 2) {
                                throw std::logic_error("Failed to build path");
                        }
                        CellI cur = greedy_path.front();
                        greedy_path.pop_front();
                        
                        auto c = capture_cost(greedy_path.front());
                        if (c && (unsigned) std::max(*c, 0) > my_units(cur)) {
                                throw std::logic_error("Tried to capture city");
                        }

                        return Move{MoveType::All, cur, greedy_path.front()};
                }
        }

        Turn trahat() const {
        }

        void check(const Turn &a) const
        {
                if (std::holds_alternative<Skip>(a)) {
                        return;
                } else if (std::holds_alternative<Move>(a)) {
                        const auto &[type, from, to] = std::get<Move>(a);
                        if (my_units(from) == 0) {
                                throw std::logic_error("Invalid turn: no units in source");
                        }
                        if (field.dist(from, to) != 1) {
                                throw std::logic_error("Invalid turn: sources does not neighbor destination");
                        }
                        const auto &c = field.at(to);
                        if (!std::holds_alternative<Visible>(c) || !std::holds_alternative<Armed>(std::get<Visible>(c))) {
                                throw std::logic_error("Invalid turn: unarmed destination");
                        }
                } else {
                        throw std::logic_error("Invalid turn: empty turn");
                }
        }

        Turn do_turn()
        {
                if (turn_num <= 300) {
                        return greedy_start();
                } else {
                        return Skip{};
                }
        }
};

struct Interactor {
        std::istream &in;
        std::ostream &out;

        void run()
        {
                unsigned n, m, k, id;
                std::cin >> n >> m >> k >> id;

                State state(n, m, k, id);

                while (true) {
                        int is_ok;
                        in >> is_ok;
                        if (!is_ok) {
                                break;
                        }
                        state.read_next(in);
                        try {
                                const auto turn = state.do_turn();
                                state.check(turn);
                                out << turn;
                        } catch (std::exception &e) {
                                std::cerr << e.what() << "\n";
                                out << Turn{Skip{}};
                        }
                }
        }
};

int main()
{
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);
        std::cerr.tie(nullptr);

        Interactor inter{std::cin, std::cout};

        inter.run();
}
