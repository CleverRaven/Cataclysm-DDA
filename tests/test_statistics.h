#ifndef TEST_STATISTICS_H
#define TEST_STATISTICS_H

class statistics {
private:
    int _types;
    int _n;
    int _sum;
    int _max;
    int _min;
    std::vector<int> samples;

public:
    statistics() : _types(0), _n(0), _sum(0), _max(INT_MIN), _min(INT_MAX) {}

    void new_type() {
        _types++;
    }
    void add( int new_val ) {
        _n++;
        _sum += new_val;
        _max = std::max(_max, new_val);
        _min = std::min(_min, new_val);
        samples.push_back(new_val);
    }
    int types() const { return _types; }
    int sum() const { return _sum; }
    int max() const { return _max; }
    int min() const { return _min; }
    float avg() const { return (float)_sum / (float)_n; }
    int n() const { return _n; }
    std::vector<int> get_samples() { return samples; }
};

#endif
