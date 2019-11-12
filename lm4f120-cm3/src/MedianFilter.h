#ifndef MedianFilter_h
#define MedianFilter_h

template <typename T, int S>
class MedianFilter
{
public:

    /* Constructor
     */
    MedianFilter()
        : m_idx(0), m_med(0), m_cnt(0)
    {
    }

    /* addSample(s): adds the sample S to the window and computes the median
     * if enough samples have been gathered
     */
    void addSample(T s)
    {
        m_buf[m_idx] = s;
        m_idx = (m_idx + 1) % S;
        m_cnt += (m_cnt < S) ? 1 : 0;
        if(m_cnt == S) {
            p_calcMedian();
        }
    }

    /* isReady(): returns true if at least the required number of samples
     * have been gathered, false otherwise
     */
    bool isReady()
    {
        return m_cnt == S;
    }

    /* getMedian(): returns the median computed when the last sample was
     * added. Does not return anything meaningful if not enough samples
     * have been gathered; check isReady() first.
     */
    T getMedian()
    {
        return m_med;
    }


private:

    int m_idx, m_med,m_cnt;
    T m_buf[S], m_tmp[S];

    /* p_calcMedian(): helper to calculate the median. Copies
     * the buffer into the temp area, then calls Hoare's in-place
     * selection algorithm to obtain the median.
     */
    void p_calcMedian()
    {
        for(int i = 0; i < S; i++) {
            m_tmp[i] = m_buf[i];
        }
        m_med = p_select(0, S - 1, S / 2);
    }

    /* p_partition(l, r, p): partition function, like from quicksort.
     * l and r are the left and right bounds of the array (m_tmp),
     * respectively, and p is the pivot index.
     */
    int p_partition(int l, int r, int p)
    {
        T tmp;
        T pv = m_tmp[p];
        m_tmp[p] = m_tmp[r];
        m_tmp[r] = pv;
        int s = l;
        for(int i = l; i < r; i++) {
            if(m_tmp[i] < pv) {
                tmp = m_tmp[s];
                m_tmp[s] = m_tmp[i];
                m_tmp[i] = tmp;
                s++;
            }
        }
        tmp = m_tmp[s];
        m_tmp[s] = m_tmp[r];
        m_tmp[r] = tmp;
        return s;
    }

    /* p_select(l, r, k): Hoare's quickselect. l and r are the
     * array bounds, and k conveys that we want to return
     * the k-th value
     */
    T p_select(int l, int r, int k)
    {
        if(l == r) {
            return m_tmp[l];
        }
        int p = (l + r) / 2;
        p = p_partition(l, r, p);
        if(p == k) {
            return m_tmp[k];
        } else if(k < p) {
            return p_select(l, p - 1, k);
        } else {
            return p_select(p + 1, r, k);
        }
    }

};

#endif
