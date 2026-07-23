#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace halfmoon {

constexpr int STARTING_BALANCE = 500;
constexpr int SMALL_BLIND = 5;
constexpr int BIG_BLIND = 10;
constexpr int STANDARD_WAGER = 10;

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

int readInt(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) throw std::runtime_error("Input stream closed.");
        try {
            std::size_t used = 0;
            const int value = std::stoi(trim(line), &used);
            if (used == trim(line).size() && value >= minimum && value <= maximum) return value;
        } catch (...) {
        }
        std::cout << "Please enter a number from " << minimum << " to " << maximum << ".\n";
    }
}

std::string readName(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string name;
        if (!std::getline(std::cin, name)) throw std::runtime_error("Input stream closed.");
        name = trim(name);
        if (name.size() >= 2 && name.size() <= 30 &&
            name.find_first_of("\r\n,") == std::string::npos) {
            return name;
        }
        std::cout << "Use 2-30 characters and do not include commas.\n";
    }
}

std::string csvEscape(const std::string& value) {
    if (value.find_first_of(",\"\n") == std::string::npos) return value;
    std::string result = "\"";
    for (char ch : value) {
        if (ch == '\"') result += '\"';
        result += ch;
    }
    return result + "\"";
}

std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '\"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '\"') {
                field += '\"';
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (ch == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
        } else {
            field += ch;
        }
    }
    fields.push_back(field);
    return fields;
}

std::string timestampNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

enum class Suit { Hearts, Diamonds, Spades, Clubs, None };
enum class Rank {
    Joker = 0,
    Two = 2,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace,
    Duke,
    Duchess,
    Reaper
};

struct Card {
    Suit suit{Suit::None};
    Rank rank{Rank::Joker};

    bool isJoker() const { return rank == Rank::Joker; }
    bool isSpecialty() const {
        return rank == Rank::Duke || rank == Rank::Duchess || rank == Rank::Reaper;
    }

    int specialtyPriority() const {
        if (rank == Rank::Reaper) return 3;
        if (rank == Rank::Duchess) return 2;
        if (rank == Rank::Duke) return 1;
        return 0;
    }

    int pokerRank() const {
        // Digital table ruling: specialty cards share the poker rank of a court card.
        if (rank == Rank::Duke) return 11;     // Jack equivalent
        if (rank == Rank::Duchess) return 12; // Queen equivalent
        if (rank == Rank::Reaper) return 13;  // King equivalent
        return static_cast<int>(rank);
    }

    int pointValue(bool aceHigh = false) const {
        if (rank == Rank::Ace) return aceHigh ? 11 : 1;
        const int value = static_cast<int>(rank);
        if (value >= 10 || isSpecialty()) return 10;
        return value;
    }

    std::string suitName() const {
        switch (suit) {
            case Suit::Hearts: return "Red Hearts";
            case Suit::Diamonds: return "Gold Diamonds";
            case Suit::Spades: return "Purple Spades";
            case Suit::Clubs: return "Silver Clubs";
            case Suit::None: return "No Suit";
        }
        return "Unknown";
    }

    std::string suitSymbol() const {
        switch (suit) {
            case Suit::Hearts: return "H";
            case Suit::Diamonds: return "D";
            case Suit::Spades: return "S";
            case Suit::Clubs: return "C";
            case Suit::None: return "*";
        }
        return "?";
    }

    std::string rankName() const {
        switch (rank) {
            case Rank::Joker: return "JOKER";
            case Rank::Two: return "2";
            case Rank::Three: return "3";
            case Rank::Four: return "4";
            case Rank::Five: return "5";
            case Rank::Six: return "6";
            case Rank::Seven: return "7";
            case Rank::Eight: return "8";
            case Rank::Nine: return "9";
            case Rank::Ten: return "10";
            case Rank::Jack: return "Jack";
            case Rank::Queen: return "Queen";
            case Rank::King: return "King";
            case Rank::Ace: return "Ace";
            case Rank::Duke: return "Duke";
            case Rank::Duchess: return "Duchess";
            case Rank::Reaper: return "Reaper";
        }
        return "Unknown";
    }

    std::string shortName() const {
        if (isJoker()) return "[JOKER]";
        return "[" + rankName() + "-" + suitSymbol() + "]";
    }

    std::string fullName() const {
        if (isJoker()) return "Joker";
        return rankName() + " of " + suitName();
    }

    std::string uniqueKey() const {
        return std::to_string(static_cast<int>(suit)) + ":" +
               std::to_string(static_cast<int>(rank));
    }
};

class Deck {
public:
    explicit Deck(std::mt19937& engine) : engine_(engine) { reset(); }

    void reset() {
        cards_.clear();
        const std::array<Suit, 4> suits{Suit::Hearts, Suit::Diamonds, Suit::Spades, Suit::Clubs};
        for (Suit suit : suits) {
            for (int value = static_cast<int>(Rank::Two);
                 value <= static_cast<int>(Rank::Ace); ++value) {
                cards_.push_back({suit, static_cast<Rank>(value)});
            }
            cards_.push_back({suit, Rank::Duke});
            cards_.push_back({suit, Rank::Duchess});
            cards_.push_back({suit, Rank::Reaper});
        }
        cards_.push_back({Suit::None, Rank::Joker});
        cards_.push_back({Suit::None, Rank::Joker});
        std::shuffle(cards_.begin(), cards_.end(), engine_);
    }

    Card draw() {
        if (cards_.empty()) throw std::runtime_error("The deck ran out of cards.");
        Card card = cards_.back();
        cards_.pop_back();
        return card;
    }

    std::size_t size() const { return cards_.size(); }
    const std::vector<Card>& cards() const { return cards_; }

private:
    std::vector<Card> cards_;
    std::mt19937& engine_;
};

struct HandValue {
    int category{-1};
    int specialty{0};
    std::vector<int> tiebreak;
    std::vector<Card> cards;
    int points{0};

    std::string categoryName() const {
        static const std::array<std::string, 10> names{
            "High Card", "One Pair", "Two Pair", "Three of a Kind", "Straight",
            "Flush", "Full House", "Four of a Kind", "Straight Flush", "Royal Flush"};
        if (category < 0 || category >= static_cast<int>(names.size())) return "No Valid Hand";
        return names[category];
    }
};

int compareHandValues(const HandValue& left, const HandValue& right) {
    if (left.category != right.category) return left.category < right.category ? -1 : 1;
    if (left.specialty != right.specialty) return left.specialty < right.specialty ? -1 : 1;
    if (left.tiebreak != right.tiebreak) {
        return std::lexicographical_compare(left.tiebreak.begin(), left.tiebreak.end(),
                                            right.tiebreak.begin(), right.tiebreak.end()) ? -1 : 1;
    }
    return 0;
}

class HandEvaluator {
public:
    static int pointsForFive(const std::vector<Card>& cards) {
        int total = 0;
        int aces = 0;
        for (const Card& card : cards) {
            if (card.rank == Rank::Ace) {
                ++aces;
                total += 1;
            } else {
                total += card.pointValue();
            }
        }
        if (aces > 0 && total + 10 <= 31) total += 10;
        return total;
    }

    static HandValue evaluateFive(const std::vector<Card>& cards) {
        if (cards.size() != 5) throw std::runtime_error("A poker hand must contain five cards.");
        HandValue result;
        result.cards = cards;
        result.points = pointsForFive(cards);

        std::map<int, int> counts;
        std::vector<int> ranks;
        int specialty = 0;
        bool flush = true;
        const Suit firstSuit = cards.front().suit;
        for (const Card& card : cards) {
            if (card.isJoker()) return result;
            ++counts[card.pokerRank()];
            ranks.push_back(card.pokerRank());
            specialty = std::max(specialty, card.specialtyPriority());
            if (card.suit != firstSuit) flush = false;
        }
        result.specialty = specialty;

        std::sort(ranks.begin(), ranks.end(), std::greater<int>());
        std::set<int> uniqueRanks(ranks.begin(), ranks.end());
        int straightHigh = 0;
        if (uniqueRanks.size() == 5) {
            std::vector<int> ascending(uniqueRanks.begin(), uniqueRanks.end());
            bool sequential = true;
            for (std::size_t i = 1; i < ascending.size(); ++i) {
                if (ascending[i] != ascending[i - 1] + 1) sequential = false;
            }
            if (sequential) straightHigh = ascending.back();
            if (ascending == std::vector<int>({2, 3, 4, 5, 14})) straightHigh = 5;
        }

        std::vector<std::pair<int, int>> groups;
        for (const auto& [rank, count] : counts) groups.emplace_back(count, rank);
        std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) {
            if (a.first != b.first) return a.first > b.first;
            return a.second > b.second;
        });

        if (flush && straightHigh == 14) {
            result.category = 9;
            result.tiebreak = {14};
        } else if (flush && straightHigh > 0) {
            result.category = 8;
            result.tiebreak = {straightHigh};
        } else if (groups.front().first >= 4) {
            result.category = 7;
            const int fourRank = groups.front().second;
            int kicker = 0;
            for (int rank : ranks) if (rank != fourRank) kicker = std::max(kicker, rank);
            result.tiebreak = {fourRank, kicker};
        } else if (groups.size() >= 2 && groups[0].first == 3 && groups[1].first >= 2) {
            result.category = 6;
            result.tiebreak = {groups[0].second, groups[1].second};
        } else if (flush) {
            result.category = 5;
            result.tiebreak = ranks;
        } else if (straightHigh > 0) {
            result.category = 4;
            result.tiebreak = {straightHigh};
        } else if (groups.front().first == 3) {
            result.category = 3;
            result.tiebreak = {groups.front().second};
            for (int rank : ranks) if (rank != groups.front().second) result.tiebreak.push_back(rank);
        } else if (groups.size() >= 2 && groups[0].first == 2 && groups[1].first == 2) {
            result.category = 2;
            const int highPair = std::max(groups[0].second, groups[1].second);
            const int lowPair = std::min(groups[0].second, groups[1].second);
            int kicker = 0;
            for (int rank : ranks) if (rank != highPair && rank != lowPair) kicker = std::max(kicker, rank);
            result.tiebreak = {highPair, lowPair, kicker};
        } else if (groups.front().first == 2) {
            result.category = 1;
            result.tiebreak = {groups.front().second};
            for (int rank : ranks) if (rank != groups.front().second) result.tiebreak.push_back(rank);
        } else {
            result.category = 0;
            result.tiebreak = ranks;
        }
        return result;
    }

    static std::optional<HandValue> bestHand(const std::vector<Card>& available,
                                             bool enforceThirtyOne) {
        if (available.size() < 5) return std::nullopt;
        std::optional<HandValue> best;
        const std::size_t n = available.size();
        for (std::size_t a = 0; a + 4 < n; ++a)
            for (std::size_t b = a + 1; b + 3 < n; ++b)
                for (std::size_t c = b + 1; c + 2 < n; ++c)
                    for (std::size_t d = c + 1; d + 1 < n; ++d)
                        for (std::size_t e = d + 1; e < n; ++e) {
                            std::vector<Card> five{available[a], available[b], available[c],
                                                   available[d], available[e]};
                            HandValue value = evaluateFive(five);
                            if (value.category < 0) continue;
                            if (enforceThirtyOne && value.points > 31) continue;
                            if (!best || compareHandValues(value, *best) > 0) best = value;
                        }
        return best;
    }
};

struct PlayerProfile {
    std::string name;
    int balance{STARTING_BALANCE};
    int wins{0};
    int losses{0};
    int ties{0};
    int folds{0};
    int jokerLosses{0};
    int hits{0};
    int busts{0};
    int gamesPlayed{0};
};

class DataStore {
public:
    explicit DataStore(fs::path directory)
        : directory_(std::move(directory)), profilesFile_(directory_ / "players.csv"),
          historyFile_(directory_ / "game_history.csv") {
        fs::create_directories(directory_);
        ensureFiles();
        load();
    }

    std::vector<PlayerProfile>& profiles() { return profiles_; }
    const std::vector<PlayerProfile>& profiles() const { return profiles_; }

    int findByName(const std::string& name) const {
        const std::string target = lower(trim(name));
        for (std::size_t i = 0; i < profiles_.size(); ++i) {
            if (lower(profiles_[i].name) == target) return static_cast<int>(i);
        }
        return -1;
    }

    void save() const {
        const fs::path temporary = profilesFile_.string() + ".tmp";
        std::ofstream out(temporary);
        if (!out) throw std::runtime_error("Could not save player profiles.");
        out << "name,balance,wins,losses,ties,folds,joker_losses,hits,busts,games_played\n";
        for (const PlayerProfile& profile : profiles_) {
            out << csvEscape(profile.name) << ',' << profile.balance << ',' << profile.wins << ','
                << profile.losses << ',' << profile.ties << ',' << profile.folds << ','
                << profile.jokerLosses << ',' << profile.hits << ',' << profile.busts << ','
                << profile.gamesPlayed << '\n';
        }
        out.close();
        if (!out) throw std::runtime_error("The player profile write did not finish.");
        std::error_code error;
        fs::remove(profilesFile_, error);
        fs::rename(temporary, profilesFile_);
    }

    void appendHistory(const PlayerProfile& profile, const std::string& result,
                       int wagered, int payout, const std::string& detail) const {
        std::ofstream out(historyFile_, std::ios::app);
        if (!out) throw std::runtime_error("Could not append game history.");
        out << csvEscape(timestampNow()) << ',' << csvEscape(profile.name) << ','
            << csvEscape(result) << ',' << wagered << ',' << payout << ',' << profile.balance << ','
            << csvEscape(detail) << '\n';
    }

    std::vector<std::vector<std::string>> recentHistory(const std::string& player,
                                                         std::size_t limit = 10) const {
        std::ifstream in(historyFile_);
        std::vector<std::vector<std::string>> matches;
        std::string line;
        std::getline(in, line);
        while (std::getline(in, line)) {
            auto fields = parseCsvLine(line);
            if (fields.size() >= 7 && lower(fields[1]) == lower(player)) matches.push_back(fields);
        }
        if (matches.size() > limit) matches.erase(matches.begin(), matches.end() - static_cast<long>(limit));
        std::reverse(matches.begin(), matches.end());
        return matches;
    }

private:
    fs::path directory_;
    fs::path profilesFile_;
    fs::path historyFile_;
    std::vector<PlayerProfile> profiles_;

    void ensureFiles() {
        if (!fs::exists(profilesFile_)) {
            std::ofstream out(profilesFile_);
            out << "name,balance,wins,losses,ties,folds,joker_losses,hits,busts,games_played\n";
        }
        if (!fs::exists(historyFile_)) {
            std::ofstream out(historyFile_);
            out << "timestamp,player,result,wagered,payout,balance_after,detail\n";
        }
    }

    void load() {
        profiles_.clear();
        std::ifstream in(profilesFile_);
        std::string line;
        std::getline(in, line);
        while (std::getline(in, line)) {
            if (trim(line).empty()) continue;
            auto fields = parseCsvLine(line);
            if (fields.size() != 10) continue;
            try {
                PlayerProfile profile;
                profile.name = fields[0];
                profile.balance = std::stoi(fields[1]);
                profile.wins = std::stoi(fields[2]);
                profile.losses = std::stoi(fields[3]);
                profile.ties = std::stoi(fields[4]);
                profile.folds = std::stoi(fields[5]);
                profile.jokerLosses = std::stoi(fields[6]);
                profile.hits = std::stoi(fields[7]);
                profile.busts = std::stoi(fields[8]);
                profile.gamesPlayed = std::stoi(fields[9]);
                if (!trim(profile.name).empty()) profiles_.push_back(profile);
            } catch (...) {
                std::cerr << "Warning: skipped a damaged player record.\n";
            }
        }
    }
};

struct RoundState {
    std::vector<Card> humanHole;
    std::vector<Card> houseHole;
    std::vector<Card> community;
    bool humanHit{false};
    bool houseHit{false};
    bool humanEliminated{false};
    bool houseEliminated{false};
    bool humanSkipFinal{false};
    bool houseSkipFinal{false};
    bool reversed{false};
    int answerMultiplier{1};
    int pot{0};
    int humanWagered{0};
};

class HalfMoonGame {
public:
    HalfMoonGame(std::mt19937& engine, DataStore& store, bool automatic = false)
        : engine_(engine), store_(store), automatic_(automatic) {}

    void play(PlayerProfile& player) {
        if (player.balance < BIG_BLIND) {
            std::cout << "\nYour balance is below the big blind. The casino grants a 100-chip recovery stake.\n";
            player.balance += 100;
        }

        RoundState state;
        Deck deck(engine_);
        const bool humanDealer = player.gamesPlayed % 2 == 0;

        banner("NEW HAND");
        std::cout << "Dealer button: " << (humanDealer ? player.name : "The House") << '\n';
        if (humanDealer) {
            pay(player, state, SMALL_BLIND);
            state.pot += BIG_BLIND;
            std::cout << player.name << " posts the small blind (5). The House posts 10.\n";
        } else {
            pay(player, state, BIG_BLIND);
            state.pot += SMALL_BLIND;
            std::cout << "The House posts the small blind (5). " << player.name << " posts 10.\n";
        }
        std::cout << "Pot: " << state.pot << " | Balance: " << player.balance << "\n";

        for (int i = 0; i < 2; ++i) {
            state.humanHole.push_back(deck.draw());
            if (state.humanHole.back().isJoker()) {
                std::cout << "\n" << player.name << " is dealt [JOKER]. The reminder has arrived.\n";
                state.humanEliminated = true;
                finish(player, state, "JOKER LOSS", 0, "Joker dealt in hole cards");
                return;
            }
            state.houseHole.push_back(deck.draw());
            if (state.houseHole.back().isJoker()) state.houseEliminated = true;
        }
        showTable(player, state, false);
        if (state.houseEliminated) {
            std::cout << "The House reveals [JOKER]. Its hand ends immediately.\n";
            finish(player, state, "WIN", state.pot, "House eliminated by a Joker");
            return;
        }

        if (!bettingRound(player, state, "PRE-FLOP")) return;
        if (!revealCommunity(deck, player, state, 3, "FLOP")) return;
        if (!bettingRound(player, state, "FLOP BETTING")) return;
        if (!revealCommunity(deck, player, state, 1, "TURN")) return;
        if (!bettingRound(player, state, "TURN BETTING")) return;
        if (!revealCommunity(deck, player, state, 1, "RIVER")) return;

        processHits(deck, player, state);
        if (state.humanEliminated || state.houseEliminated) {
            if (state.humanEliminated) {
                finish(player, state, "JOKER LOSS", 0, "Joker drawn after declaring Hit");
            } else {
                finish(player, state, "WIN", state.pot, "House drew a Joker after declaring Hit");
            }
            return;
        }

        resolveSpecialties(deck, player, state, humanDealer);
        if (state.humanEliminated || state.houseEliminated) {
            if (state.humanEliminated) {
                finish(player, state, "JOKER LOSS", 0, "Reaper forced a Joker draw");
            } else {
                finish(player, state, "WIN", state.pot, "Reaper eliminated the House");
            }
            return;
        }

        if (!finalBetting(player, state)) return;
        showdown(player, state);
    }

private:
    std::mt19937& engine_;
    DataStore& store_;
    bool automatic_{false};

    static void banner(const std::string& title) {
        std::cout << "\n============================================================\n"
                  << "  " << title << "\n"
                  << "============================================================\n";
    }

    bool chance(int percent) {
        std::uniform_int_distribution<int> distribution(1, 100);
        return distribution(engine_) <= percent;
    }

    void pay(PlayerProfile& player, RoundState& state, int amount) {
        amount = std::min(amount, player.balance);
        player.balance -= amount;
        state.humanWagered += amount;
        state.pot += amount;
    }

    static std::string cardsText(const std::vector<Card>& cards) {
        std::ostringstream out;
        for (const Card& card : cards) out << card.shortName() << ' ';
        return out.str();
    }

    static void showTable(const PlayerProfile& player, const RoundState& state, bool revealHouse) {
        std::cout << "\n" << player.name << "'s hole cards: " << cardsText(state.humanHole) << '\n';
        std::cout << "Community: " << (state.community.empty() ? "(none)" : cardsText(state.community)) << '\n';
        std::cout << "House: " << (revealHouse ? cardsText(state.houseHole) : "[hidden] [hidden]") << '\n';
        std::cout << "Pot: " << state.pot << " | Balance: " << player.balance << "\n";
    }

    bool bettingRound(PlayerProfile& player, RoundState& state, const std::string& stage) {
        banner(stage);
        showTable(player, state, false);
        const bool houseOpens = chance(35);
        if (houseOpens) {
            std::cout << "The House bets " << STANDARD_WAGER << ".\n";
            state.pot += STANDARD_WAGER;
            const int call = std::min(STANDARD_WAGER, player.balance);
            const int choice = automatic_ ? 1 : readInt("1. Call  2. Fold\nChoice: ", 1, 2);
            if (choice == 2) {
                player.folds++;
                finish(player, state, "FOLD", 0, "Folded during " + stage);
                return false;
            }
            pay(player, state, call);
            std::cout << player.name << " calls " << call << ".\n";
        } else {
            const int choice = automatic_ ? (player.balance >= STANDARD_WAGER ? 2 : 1)
                                          : readInt("1. Check  2. Bet 10  3. Fold\nChoice: ", 1, 3);
            if (choice == 3) {
                player.folds++;
                finish(player, state, "FOLD", 0, "Folded during " + stage);
                return false;
            }
            if (choice == 2 && player.balance > 0) {
                pay(player, state, STANDARD_WAGER);
                state.pot += STANDARD_WAGER;
                std::cout << player.name << " bets. The House calls.\n";
            } else {
                std::cout << "Both players check.\n";
            }
        }
        return true;
    }

    bool revealCommunity(Deck& deck, PlayerProfile& player, RoundState& state,
                         int count, const std::string& stage) {
        banner(stage);
        for (int i = 0; i < count; ++i) {
            const Card card = deck.draw();
            std::cout << "Revealed: " << card.fullName() << '\n';
            if (card.isJoker()) {
                std::cout << "A community Joker kills the hand. All of your bets are returned.\n";
                player.balance += state.humanWagered;
                const int refunded = state.humanWagered;
                state.humanWagered = 0;
                state.pot = 0;
                finish(player, state, "DEAD HAND", refunded, "Community Joker; bets refunded", false);
                return false;
            }
            state.community.push_back(card);
        }
        showTable(player, state, false);
        return true;
    }

    void processHits(Deck& deck, PlayerProfile& player, RoundState& state) {
        banner("THE HIT");
        showTable(player, state, false);
        const int choice = automatic_ ? 2 : readInt("Declare 'Hit me'?\n1. Hit me  2. Stand\nChoice: ", 1, 2);
        if (choice == 1) {
            state.humanHit = true;
            player.hits++;
            const Card card = deck.draw();
            std::cout << player.name << " draws " << card.fullName() << ".\n";
            state.humanHole.push_back(card);
            if (card.isJoker()) state.humanEliminated = true;
        }

        int houseHolePoints = 0;
        for (const Card& card : state.houseHole) houseHolePoints += card.pointValue();
        if (houseHolePoints <= 14 || chance(20)) {
            state.houseHit = true;
            const Card card = deck.draw();
            std::cout << "The House says, 'Hit me.' Its card remains hidden.\n";
            state.houseHole.push_back(card);
            if (card.isJoker()) state.houseEliminated = true;
        } else {
            std::cout << "The House stands. It pretends this was an easy decision.\n";
        }
    }

    struct Trigger {
        bool humanOwner;
        Card card;
    };

    void resolveSpecialties(Deck& deck, const PlayerProfile& player, RoundState& state,
                            bool humanDealer) {
        banner("SPECIALTY RESOLUTION");
        std::vector<Trigger> triggers;
        auto addTriggers = [&triggers](bool human, const std::vector<Card>& cards) {
            for (const Card& card : cards) if (card.isSpecialty()) triggers.push_back({human, card});
        };
        // Clockwise from the dealer: in heads-up play, the non-dealer resolves first.
        if (humanDealer) {
            addTriggers(false, state.houseHole);
            addTriggers(true, state.humanHole);
        } else {
            addTriggers(true, state.humanHole);
            addTriggers(false, state.houseHole);
        }

        if (triggers.empty()) {
            std::cout << "No privately held specialty cards activate. The table is briefly normal.\n";
            return;
        }

        for (const Trigger& trigger : triggers) {
            const std::string owner = trigger.humanOwner ? player.name : "The House";
            const std::string target = trigger.humanOwner ? "The House" : player.name;
            std::cout << owner << " reveals " << trigger.card.fullName() << ".\n";
            if (trigger.card.rank == Rank::Duke) {
                if (trigger.humanOwner) state.houseSkipFinal = true;
                else state.humanSkipFinal = true;
                std::cout << target << " must skip the final betting round.\n";
            } else if (trigger.card.rank == Rank::Duchess) {
                state.reversed = !state.reversed;
                state.answerMultiplier = 2;
                std::cout << "Betting order reverses; every heads-up wager must be answered twice.\n";
            } else if (trigger.card.rank == Rank::Reaper) {
                std::cout << target << " draws two Reaper cards: ";
                for (int i = 0; i < 2; ++i) {
                    const Card drawn = deck.draw();
                    std::cout << drawn.shortName() << ' ';
                    if (drawn.isJoker()) {
                        if (trigger.humanOwner) state.houseEliminated = true;
                        else state.humanEliminated = true;
                    }
                }
                std::cout << "\nThe cards are discarded; only a Joker matters. Emotional damage remains.\n";
            }
            if (state.humanEliminated || state.houseEliminated) break;
        }
    }

    bool finalBetting(PlayerProfile& player, RoundState& state) {
        banner("FINAL BETTING");
        if (state.humanSkipFinal || state.houseSkipFinal) {
            std::cout << "A Duke closes action for this heads-up round. Both players check through.\n";
            return true;
        }

        const bool houseFirst = state.reversed;
        const int callAmount = STANDARD_WAGER * state.answerMultiplier;
        if (houseFirst) {
            std::cout << "The reversed order makes the House act first.\n";
            if (chance(55)) {
                std::cout << "The House wagers " << STANDARD_WAGER << "; the answer costs "
                          << callAmount << ".\n";
                state.pot += STANDARD_WAGER;
                const int choice = automatic_ ? 1 : readInt("1. Answer  2. Fold\nChoice: ", 1, 2);
                if (choice == 2) {
                    player.folds++;
                    finish(player, state, "FOLD", 0, "Folded during final betting");
                    return false;
                }
                pay(player, state, callAmount);
            } else {
                std::cout << "The House checks.\n";
            }
        } else {
            const int choice = automatic_ ? 1
                : readInt("1. Check  2. Bet 10  3. Fold\nChoice: ", 1, 3);
            if (choice == 3) {
                player.folds++;
                finish(player, state, "FOLD", 0, "Folded during final betting");
                return false;
            }
            if (choice == 2 && player.balance > 0) {
                pay(player, state, STANDARD_WAGER);
                state.pot += callAmount;
                std::cout << "The House answers with " << callAmount << ".\n";
            } else {
                std::cout << "Both players check.\n";
            }
        }
        return true;
    }

    void showdown(PlayerProfile& player, RoundState& state) {
        banner("SHOWDOWN");
        showTable(player, state, true);
        std::vector<Card> humanAvailable = state.community;
        humanAvailable.insert(humanAvailable.end(), state.humanHole.begin(), state.humanHole.end());
        std::vector<Card> houseAvailable = state.community;
        houseAvailable.insert(houseAvailable.end(), state.houseHole.begin(), state.houseHole.end());

        const auto humanBest = HandEvaluator::bestHand(humanAvailable, state.humanHit);
        const auto houseBest = HandEvaluator::bestHand(houseAvailable, state.houseHit);
        if (!humanBest) {
            player.busts++;
            std::cout << player.name << " cannot build a five-card hand at 31 points or less and busts.\n";
            finish(player, state, "BUST", 0, "Hit hand exceeded 31 points");
            return;
        }
        if (!houseBest) {
            std::cout << "The House busts under the 31-point Hit limit.\n";
            finish(player, state, "WIN", state.pot, "House busted after Hit");
            return;
        }

        std::cout << player.name << ": " << humanBest->categoryName() << " | "
                  << cardsText(humanBest->cards);
        if (state.humanHit) std::cout << "| " << humanBest->points << " points";
        std::cout << '\n';
        std::cout << "The House: " << houseBest->categoryName() << " | "
                  << cardsText(houseBest->cards);
        if (state.houseHit) std::cout << "| " << houseBest->points << " points";
        std::cout << '\n';

        const int comparison = compareHandValues(*humanBest, *houseBest);
        if (comparison > 0) {
            finish(player, state, "WIN", state.pot,
                   humanBest->categoryName() + " defeated " + houseBest->categoryName());
        } else if (comparison < 0) {
            finish(player, state, "LOSS", 0,
                   houseBest->categoryName() + " defeated " + humanBest->categoryName());
        } else {
            const int share = state.pot / 2;
            finish(player, state, "TIE", share, "Pot split after exact tie");
        }
    }

    void finish(PlayerProfile& player, RoundState& state, const std::string& result,
                int payout, const std::string& detail, bool countGame = true) {
        if (payout > 0) player.balance += payout;
        if (countGame) {
            player.gamesPlayed++;
            if (result == "WIN") player.wins++;
            else if (result == "TIE") player.ties++;
            else {
                player.losses++;
                if (result == "JOKER LOSS") player.jokerLosses++;
            }
        }
        store_.save();
        store_.appendHistory(player, result, state.humanWagered, payout, detail);
        std::cout << "\nRESULT: " << result << '\n'
                  << "Wagered: " << state.humanWagered << " | Payout: " << payout
                  << " | New balance: " << player.balance << "\n"
                  << detail << "\n";
    }
};

void printRules() {
    std::cout << R"RULES(
===================== HALF-MOON HOLD 'EM RULES =====================
Deck: 52 standard cards + Duke, Duchess, and Reaper in each suit + 2 Jokers = 66.

JOKERS
  Player draw: that player's hand ends; committed bets stay in the pot.
  Community draw: the hand dies and the human player's wagers are refunded.

PLAY
  Heads-up Hold 'Em against the House. Dealer rotates every counted hand.
  Two hole cards, then Flop (3), Turn (1), and River (1).
  The best five-card poker hand wins.

SPECIALTY EFFECTS (resolve after the River and Hit, before final betting)
  Duke: opponent skips the final betting round.
  Duchess: reverses order; a heads-up wager must be answered at twice its amount.
  Reaper: opponent draws two trap cards. A Joker eliminates them; otherwise both
          cards are discarded.

THE HIT
  After the River, either player may draw one usable hole card. A player who Hits
  must form a five-card poker hand worth 31 points or less. Aces count as 1 or 11;
  number cards use face value; all court and specialty cards count as 10.

DIGITAL TABLE RULINGS
  Duke/Duchess/Reaper are Jack/Queen/King equivalents for poker combinations.
  Specialty priority breaks equal hand categories before normal rank comparison:
  Reaper > Duchess > Duke. Only privately held specialty cards trigger effects;
  specialty cards in the community still count toward every player's poker hand.
====================================================================
)RULES";
}

void printProfile(const PlayerProfile& profile) {
    const int completed = profile.wins + profile.losses + profile.ties;
    const double winRate = completed == 0 ? 0.0 : 100.0 * profile.wins / completed;
    std::cout << "\n---------------------- PLAYER PROFILE ----------------------\n"
              << "Name: " << profile.name << '\n'
              << "Balance: " << profile.balance << " chips\n"
              << "Games: " << profile.gamesPlayed << " | Wins: " << profile.wins
              << " | Losses: " << profile.losses << " | Ties: " << profile.ties << '\n'
              << "Win rate: " << std::fixed << std::setprecision(1) << winRate << "%\n"
              << "Folds: " << profile.folds << " | Hits: " << profile.hits
              << " | 31-busts: " << profile.busts
              << " | Joker losses: " << profile.jokerLosses << "\n"
              << "------------------------------------------------------------\n";
}

void printLeaderboard(const DataStore& store) {
    std::vector<PlayerProfile> sorted = store.profiles();
    std::sort(sorted.begin(), sorted.end(), [](const PlayerProfile& left, const PlayerProfile& right) {
        if (left.balance != right.balance) return left.balance > right.balance;
        if (left.wins != right.wins) return left.wins > right.wins;
        return lower(left.name) < lower(right.name);
    });
    std::cout << "\n====================== LEADERBOARD ======================\n";
    if (sorted.empty()) {
        std::cout << "No player records yet.\n";
        return;
    }
    std::cout << std::left << std::setw(5) << "#" << std::setw(24) << "Player"
              << std::right << std::setw(10) << "Chips" << std::setw(8) << "Wins"
              << std::setw(8) << "Games" << '\n';
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        std::cout << std::left << std::setw(5) << i + 1 << std::setw(24) << sorted[i].name
                  << std::right << std::setw(10) << sorted[i].balance << std::setw(8)
                  << sorted[i].wins << std::setw(8) << sorted[i].gamesPlayed << '\n';
    }
}

void printHistory(const DataStore& store, const PlayerProfile& profile) {
    const auto history = store.recentHistory(profile.name);
    std::cout << "\n==================== RECENT GAME HISTORY ====================\n";
    if (history.empty()) {
        std::cout << "No saved hands for " << profile.name << ".\n";
        return;
    }
    for (const auto& fields : history) {
        std::cout << fields[0] << " | " << std::setw(10) << fields[2]
                  << " | wager " << fields[3] << " | payout " << fields[4]
                  << " | balance " << fields[5] << "\n  " << fields[6] << '\n';
    }
}

void manageProfiles(DataStore& store, int& selected) {
    while (true) {
        std::cout << "\n===================== PLAYER RECORDS =====================\n"
                  << "1. Create player\n2. Select player\n3. Search player\n"
                  << "4. Rename selected player\n5. Delete selected player\n6. Back\n";
        const int choice = readInt("Choice: ", 1, 6);
        if (choice == 6) return;
        if (choice == 1) {
            const std::string name = readName("New player name: ");
            if (store.findByName(name) >= 0) {
                std::cout << "That player already exists.\n";
            } else {
                store.profiles().push_back(PlayerProfile{name});
                selected = static_cast<int>(store.profiles().size()) - 1;
                store.save();
                std::cout << "Created and selected " << name << " with "
                          << STARTING_BALANCE << " chips.\n";
            }
        } else if (choice == 2) {
            if (store.profiles().empty()) {
                std::cout << "No players exist yet.\n";
                continue;
            }
            for (std::size_t i = 0; i < store.profiles().size(); ++i) {
                std::cout << i + 1 << ". " << store.profiles()[i].name << " ("
                          << store.profiles()[i].balance << " chips)\n";
            }
            selected = readInt("Select: ", 1, static_cast<int>(store.profiles().size())) - 1;
        } else if (choice == 3) {
            const std::string query = readName("Exact player name: ");
            const int index = store.findByName(query);
            if (index < 0) std::cout << "No matching player was found.\n";
            else printProfile(store.profiles()[index]);
        } else if (choice == 4) {
            if (selected < 0) {
                std::cout << "Select a player first.\n";
                continue;
            }
            const std::string name = readName("New name: ");
            const int existing = store.findByName(name);
            if (existing >= 0 && existing != selected) {
                std::cout << "That name is already in use.\n";
            } else {
                store.profiles()[selected].name = name;
                store.save();
                std::cout << "Player record updated.\n";
            }
        } else if (choice == 5) {
            if (selected < 0) {
                std::cout << "Select a player first.\n";
                continue;
            }
            std::cout << "Delete " << store.profiles()[selected].name << "?\n";
            if (readInt("1. Yes  2. No\nChoice: ", 1, 2) == 1) {
                store.profiles().erase(store.profiles().begin() + selected);
                selected = store.profiles().empty() ? -1 : 0;
                store.save();
                std::cout << "The player profile was deleted. Existing history remains as an audit log.\n";
            }
        }
    }
}

int runSelfTests() {
    std::mt19937 engine(12345);
    Deck deck(engine);
    bool passed = true;
    auto expect = [&passed](bool condition, const std::string& label) {
        std::cout << (condition ? "[PASS] " : "[FAIL] ") << label << '\n';
        passed = passed && condition;
    };

    expect(deck.size() == 66, "Deck contains exactly 66 cards");
    std::map<std::string, int> counts;
    int jokers = 0;
    int specialties = 0;
    for (const Card& card : deck.cards()) {
        if (card.isJoker()) ++jokers;
        else ++counts[card.uniqueKey()];
        if (card.isSpecialty()) ++specialties;
    }
    expect(jokers == 2, "Deck contains two Jokers");
    expect(specialties == 12, "Deck contains twelve specialty cards");
    expect(std::all_of(counts.begin(), counts.end(), [](const auto& item) { return item.second == 1; }),
           "Every suited card is unique");

    const std::vector<Card> royal{{Suit::Hearts, Rank::Ten}, {Suit::Hearts, Rank::Jack},
                                  {Suit::Hearts, Rank::Queen}, {Suit::Hearts, Rank::King},
                                  {Suit::Hearts, Rank::Ace}};
    expect(HandEvaluator::evaluateFive(royal).categoryName() == "Royal Flush",
           "Royal flush evaluator");

    const std::vector<Card> capped{{Suit::Hearts, Rank::Ace}, {Suit::Diamonds, Rank::Ten},
                                   {Suit::Spades, Rank::Ten}, {Suit::Clubs, Rank::Five},
                                   {Suit::Hearts, Rank::Four}};
    expect(HandEvaluator::pointsForFive(capped) == 30, "Ace adjusts from 11 to 1 under the 31 cap");

    std::cout << (passed ? "All self-tests passed.\n" : "One or more self-tests failed.\n");
    return passed ? 0 : 1;
}

void runApplication(DataStore& store, std::mt19937& engine, bool demo) {
    int selected = store.profiles().empty() ? -1 : 0;
    if (demo && selected < 0) {
        store.profiles().push_back(PlayerProfile{"Demo Player"});
        selected = 0;
        store.save();
    }

    std::cout << R"TITLE(
 _   _    _    _     _____       __  __  ___   ___  _   _
| | | |  / \  | |   |  ___|     |  \/  |/ _ \ / _ \| \ | |
| |_| | / _ \ | |   | |_  _____ | |\/| | | | | | | |  \| |
|  _  |/ ___ \| |___|  _||_____| |  | | |_| | |_| | |\  |
|_| |_/_/   \_\_____|_|          |_|  |_|\___/ \___/|_| \_|
                         HOLD 'EM
)TITLE";
    std::cout << "A 66-card casino game. Winning is encouraged. Surviving the table is optional.\n";

    if (demo) {
        std::cout << "\nDEMONSTRATION MODE: choices are automated with a repeatable seed.\n";
        HalfMoonGame game(engine, store, true);
        game.play(store.profiles()[selected]);
        printProfile(store.profiles()[selected]);
        printHistory(store, store.profiles()[selected]);
        return;
    }

    while (true) {
        const std::string active = selected >= 0 ? store.profiles()[selected].name : "none";
        std::cout << "\n======================== MAIN MENU ========================\n"
                  << "Active player: " << active << '\n'
                  << "1. Play a hand\n2. View active profile\n3. View recent history\n"
                  << "4. Player records\n5. Leaderboard\n6. Rules\n7. Exit\n";
        const int choice = readInt("Choice: ", 1, 7);
        if (choice == 7) {
            store.save();
            std::cout << "Records saved. The table will remember you.\n";
            return;
        }
        if (choice == 4) {
            manageProfiles(store, selected);
            continue;
        }
        if (choice == 5) {
            printLeaderboard(store);
            continue;
        }
        if (choice == 6) {
            printRules();
            continue;
        }
        if (selected < 0) {
            std::cout << "Create or select a player in Player Records first.\n";
            continue;
        }
        if (choice == 1) {
            HalfMoonGame game(engine, store);
            game.play(store.profiles()[selected]);
        } else if (choice == 2) {
            printProfile(store.profiles()[selected]);
        } else if (choice == 3) {
            printHistory(store, store.profiles()[selected]);
        }
    }
}

} // namespace halfmoon

int main(int argc, char* argv[]) {
    try {
        fs::path dataDirectory = "data";
        bool demo = false;
        bool selfTest = false;
        std::optional<unsigned int> suppliedSeed;

        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--demo") demo = true;
            else if (argument == "--self-test") selfTest = true;
            else if (argument == "--data-dir" && i + 1 < argc) dataDirectory = argv[++i];
            else if (argument == "--seed" && i + 1 < argc) suppliedSeed = static_cast<unsigned int>(std::stoul(argv[++i]));
            else if (argument == "--help") {
                std::cout << "Usage: half_moon_holdem [--demo] [--self-test] [--seed N] [--data-dir PATH]\n";
                return 0;
            } else {
                throw std::runtime_error("Unknown or incomplete option: " + argument);
            }
        }

        if (selfTest) return halfmoon::runSelfTests();
        const unsigned int seed = suppliedSeed.value_or(std::random_device{}());
        std::mt19937 engine(seed);
        halfmoon::DataStore store(dataDirectory);
        halfmoon::runApplication(store, engine, demo);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
