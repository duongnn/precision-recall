package wcpprocess.helpers;

import java.util.Random;

/**
 * Created by duongnn on 11/5/18.
 */
public class Utilities {

    // return an integer uniformly randomly selected in the range, inclusive
    public static int getRandomNumberInRange(int lb, int ub){
        Random random;
        random = new Random();
        return random.nextInt(ub + 1 - lb) + lb;
    }
}
