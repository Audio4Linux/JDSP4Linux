package james.dsp.preference;
class Biquad {
    private Complex mB0, mB1, mB2, mA0, mA1, mA2;

    protected void setHighShelf(double centerFrequency, double samplingFrequency,
            double dbGain, double slope) {
        double w0 = 2 * Math.PI * centerFrequency / samplingFrequency;
        double a = Math.pow(10, dbGain/40);
        double alpha = Math.sin(w0) / 2 * Math.sqrt((a + 1 / a) * (1 / slope - 1) + 2);

        mB0 = new Complex(a*((a+1) + (a-1)   *Math.cos(w0) + 2*Math.sqrt(a)*alpha), 0);
        mB1 = new Complex(-2*a*((a-1) + (a+1)*Math.cos(w0)), 0);
        mB2 = new Complex(a*((a+1) + (a-1)   *Math.cos(w0) - 2*Math.sqrt(a)*alpha), 0);
        mA0 = new Complex((a+1) - (a-1)      *Math.cos(w0) + 2*Math.sqrt(a)*alpha, 0);
        mA1 = new Complex(2*((a-1) - (a+1)   *Math.cos(w0)), 0);
        mA2 = new Complex((a+1) - (a-1)      *Math.cos(w0) - 2*Math.sqrt(a)*alpha, 0);
    }

    protected Complex evaluateTransfer(Complex z) {
        Complex zSquared = z.mul(z);
        Complex nom = mB0.add(mB1.div(z)).add(mB2.div(zSquared));
        Complex den = mA0.add(mA1.div(z)).add(mA2.div(zSquared));
        return nom.div(den);
    }
}
