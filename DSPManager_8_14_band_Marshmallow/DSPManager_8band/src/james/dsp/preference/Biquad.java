package james.dsp.preference;

/**
 * Evaluate transfer functions of biquad filters in direct form 1.
 *
 * @author alankila
 */
class Biquad {
    private Complex b0, b1, b2, a0, a1, a2;

    protected void setHighShelf(double centerFrequency, double samplingFrequency, double dbGain, double slope) {
        double w0 = 2 * Math.PI * centerFrequency / samplingFrequency;
        double A = Math.pow(10, dbGain / 40);
        double alpha = Math.sin(w0) / 2 * Math.sqrt((A + 1 / A) * (1 / slope - 1) + 2);

        b0 = new Complex(A * ((A + 1) + (A - 1) * Math.cos(w0) + 2 * Math.sqrt(A) * alpha), 0);
        b1 = new Complex(-2 * A * ((A - 1) + (A + 1) * Math.cos(w0)), 0);
        b2 = new Complex(A * ((A + 1) + (A - 1) * Math.cos(w0) - 2 * Math.sqrt(A) * alpha), 0);
        a0 = new Complex((A + 1) - (A - 1) * Math.cos(w0) + 2 * Math.sqrt(A) * alpha, 0);
        a1 = new Complex(2 * ((A - 1) - (A + 1) * Math.cos(w0)), 0);
        a2 = new Complex((A + 1) - (A - 1) * Math.cos(w0) - 2 * Math.sqrt(A) * alpha, 0);
    }

    protected Complex evaluateTransfer(Complex z) {
        Complex zSquared = z.mul(z);
        Complex nom = b0.add(b1.div(z)).add(b2.div(zSquared));
        Complex den = a0.add(a1.div(z)).add(a2.div(zSquared));
        return nom.div(den);
    }
}
