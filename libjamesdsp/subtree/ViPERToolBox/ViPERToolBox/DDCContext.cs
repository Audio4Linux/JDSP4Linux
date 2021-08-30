namespace ViPERToolBox
{
    using System;
    using System.Collections.Generic;
    using System.Threading;

    public class DDCContext
    {
        private Dictionary<int, Biquad> m_lstFilterBank = new Dictionary<int, Biquad>();
        private Mutex m_mFilterMutex = new Mutex();

        public DDCContext()
        {
            this.m_lstFilterBank.Clear();
        }

        public bool AddFilter(int nFreq, double dGain, double dBandwidth, double dSRate)
        {
            this.LockFilter();
            if (!this.m_lstFilterBank.ContainsKey(nFreq))
            {
                Biquad biquad = new Biquad();
                biquad.RefreshFilter(dGain, (double) nFreq, dSRate, dBandwidth);
                this.m_lstFilterBank.Add(nFreq, biquad);
            }
            this.UnlockFilter();
            return false;
        }

        public void ClearFilters()
        {
            this.LockFilter();
            this.m_lstFilterBank.Clear();
            this.UnlockFilter();
        }

        public double[] ExportCoeffs(double dSamplingRate)
        {
            double[] numArray = null;
            this.LockFilter();
            Dictionary<int, Biquad>.Enumerator enumerator = this.m_lstFilterBank.GetEnumerator();
            List<Biquad> list = new List<Biquad>();
            while (enumerator.MoveNext())
            {
                KeyValuePair<int, Biquad> current = enumerator.Current;
                list.Add(current.Value);
            }
            if (list.Count <= 0)
            {
                this.UnlockFilter();
                return null;
            }
            numArray = new double[list.Count * 5];
            for (int i = 0; i < list.Count; i++)
            {
                double[] numArray2 = list[i].ExportCoeffs(dSamplingRate);
                if (numArray2 == null)
                {
                    list.Clear();
                    this.UnlockFilter();
                    return null;
                }
                for (int j = 0; j < 5; j++)
                {
                    numArray[(i * 5) + j] = numArray2[j];
                }
            }
            list.Clear();
            this.UnlockFilter();
            return numArray;
        }

        public float[] GetResponseTable(int nBandCount, double dSRate)
        {
            if (nBandCount <= 0)
            {
                return null;
            }
            List<Biquad> list = new List<Biquad>();
            this.LockFilter();
            Dictionary<int, Biquad>.Enumerator enumerator = this.m_lstFilterBank.GetEnumerator();
            while (enumerator.MoveNext())
            {
                KeyValuePair<int, Biquad> current = enumerator.Current;
                list.Add(current.Value);
            }
            this.UnlockFilter();
            float[] numArray = null;
            numArray = new float[nBandCount];
            for (int i = 0; i < nBandCount; i++)
            {
                numArray[i] = 0f;
            }
            for (int j = 0; j < nBandCount; j++)
            {
                double num3 = (dSRate / 2.0) / ((double) nBandCount);
                for (int k = 0; k < list.Count; k++)
                {
                    numArray[j] += (float) list[k].GainAt(num3 * (j + 1.0), dSRate);
                }
            }
            return numArray;
        }

        public void LockFilter()
        {
            this.m_mFilterMutex.WaitOne();
        }

        public void ModifyFilter(int nOldFreq, int nNewFreq, double dGain, double dBandwidth, double dSRate)
        {
            this.LockFilter();
            if (nOldFreq == nNewFreq)
            {
                if (this.m_lstFilterBank.ContainsKey(nOldFreq))
                {
                    this.m_lstFilterBank[nOldFreq].RefreshFilter(dGain, (double) nNewFreq, dSRate, dBandwidth);
                }
            }
            else if (this.m_lstFilterBank.ContainsKey(nOldFreq) && !this.m_lstFilterBank.ContainsKey(nNewFreq))
            {
                this.m_lstFilterBank.Remove(nOldFreq);
                Biquad biquad = new Biquad();
                biquad.RefreshFilter(dGain, (double) nNewFreq, dSRate, dBandwidth);
                this.m_lstFilterBank.Add(nNewFreq, biquad);
            }
            this.UnlockFilter();
        }

        public void RemoveFilter(int nFreq)
        {
            this.LockFilter();
            if (this.m_lstFilterBank.ContainsKey(nFreq))
            {
                this.m_lstFilterBank.Remove(nFreq);
            }
            this.UnlockFilter();
        }

        public void UnlockFilter()
        {
            this.m_mFilterMutex.ReleaseMutex();
        }
    }
}