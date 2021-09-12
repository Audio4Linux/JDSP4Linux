namespace ViPERToolBox
{
    using System;
    public class Biquad
    {
        private double[] internalBiquadCoeffs = new double[4];
        private double a0;
        private double m_dFilterBQ;
        private double m_dFilterFreq;
        private double m_dFilterGain;

        public Biquad()
        {
            this.internalBiquadCoeffs[0] = 0.0;
            this.internalBiquadCoeffs[1] = 0.0;
            this.internalBiquadCoeffs[2] = 0.0;
            this.internalBiquadCoeffs[3] = 0.0;
            this.a0 = 0.0;
        }
        
        public double[] ExportCoeffs(double dSamplingRate)
        {
            return this.ExportCoeffs(this.m_dFilterGain, this.m_dFilterFreq, dSamplingRate, this.m_dFilterBQ);
        }

        private double[] ExportCoeffs(double dbGain, double centreFreq, double fs, double dBandwidthOrQOrS)
        {
            if (centreFreq <= 2.2204460492503131e-016 || fs <= 2.2204460492503131e-016)
                return null;
            double d = Math.Pow(10.0, dbGain / 40.0);
            double omega = (6.2831853071795862 * centreFreq) / fs;
            double num3 = Math.Sin(omega);
            double cs = Math.Cos(omega);
            double alpha = num3 * Math.Sinh(((0.34657359027997264 * dBandwidthOrQOrS) * omega) / num3);
            double B0 = 1.0 + (alpha * d);
            double B1 = -2.0 * cs;
            double B2 = 1.0 - (alpha * d);
            double A0 = 1.0 + (alpha / d);
            double A1 = -2.0 * cs;
            double A2 = 1.0 - (alpha / d);
            return new double[] { (B0 / A0), (B1 / A0), (B2 / A0), (-A1 / A0), (-A2 / A0) };
        }

        public double GainAt(double centreFreq, double fs)
        {
            double num = (6.2831853071795862 * centreFreq) / fs;
            double num2 = Math.Sin(num / 2.0);
            double num3 = num2 * num2;
            double cs = this.a0;
            double alpha = this.internalBiquadCoeffs[0];
            double num6 = this.internalBiquadCoeffs[1];
            double A0 = 1.0;
            double A1 = -this.internalBiquadCoeffs[2];
            double A2 = -this.internalBiquadCoeffs[3];
            return ((10.0 * Math.Log10((Math.Pow((cs + alpha) + num6, 2.0) - ((4.0 * (((cs * alpha) + ((4.0 * cs) * num6)) + (alpha * num6))) * num3)) + ((((16.0 * cs) * num6) * num3) * num3))) - (10.0 * Math.Log10((Math.Pow((A0 + A1) + A2, 2.0) - ((4.0 * (((A0 * A1) + ((4.0 * A0) * A2)) + (A1 * A2))) * num3)) + ((((16.0 * A0) * A2) * num3) * num3))));
        }
        
        public void RefreshFilter(double dbGain, double centreFreq, double fs, double dBandwidthOrQOrS)
        {
            this.m_dFilterGain = dbGain;
            this.m_dFilterFreq = centreFreq;
            this.m_dFilterBQ = dBandwidthOrQOrS;
            double d = Math.Pow(10.0, dbGain / 40.0);
            double a = (6.2831853071795862 * centreFreq) / fs;
            double num3 = Math.Sin(a);
            double cs = Math.Cos(a);
            double alpha = num3 * Math.Sinh(((0.34657359027997264 * dBandwidthOrQOrS) * a) / num3);
            double B0 = 1.0 + (alpha * d);
            double B1 = -2.0 * cs;
            double B2 = 1.0 - (alpha * d);
            double A0 = 1.0 + (alpha / d);
            double A1 = -2.0 * cs;
            double A2 = 1.0 - (alpha / d);
            this.a0 = B0 / A0;
            this.internalBiquadCoeffs[0] = B1 / A0;
            this.internalBiquadCoeffs[1] = B2 / A0;
            this.internalBiquadCoeffs[2] = -A1 / A0;
            this.internalBiquadCoeffs[3] = -A2 / A0;
        }
    }
}