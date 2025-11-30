#ifndef SCORE_H
#define SCORE_H

class Score {
private:
    uint8_t scores[2];

public:
    // Constants for team indices
    static const uint8_t HOME = 0;
    static const uint8_t AWAY = 1;

    // Constructor
    Score() {
      Reset();
    }

    // Getters
    int GetHomeScore() const {
        return scores[HOME];
    }

    int GetAwayScore() const {
        return scores[AWAY];
    }

    // Setters
    void SetHomeScore(uint8_t value) {
        scores[HOME] = value;
    }

    void SetAwayScore(uint8_t value) {
        scores[AWAY] = value;
    }

    // Increment functions
    void HomeScore() {
        updateScore(HOME, 1);
    }

    void AwayScore() {
        updateScore(AWAY, 1);
    }

    // Adjust functions (can be negative)
    void HomeAdjust() {
        updateScore(HOME, -1);
    }

    void AwayAdjust() {
        updateScore(AWAY, -1);
    }

    // Reset scores
    void Reset() {
        scores[HOME] = 0;
        scores[AWAY] = 0;
    }

private:
    void updateScore(uint8_t who, int8_t qty) {
      if (scores[who] + qty < 0) {
        // Nothing to do
        return;
      }
      scores[who] += qty;
    }
};

#endif // SCORE_H