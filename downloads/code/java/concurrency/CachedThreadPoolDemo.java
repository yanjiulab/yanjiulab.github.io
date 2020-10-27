package concurrency;
import java.util.concurrent.*;

public class CachedThreadPoolDemo {
    public static void main(String[] args) {
        ExecutorService executorService = Executors.newCachedThreadPool();
        for (int i = 0; i < 4; i++) {
            executorService.execute(new TaskWithoutResult());
        }
        executorService.shutdown();
    }
}/*Output(*):
#pool-1-thread-4(2) #pool-1-thread-1(2) #pool-1-thread-3(2)
#pool-1-thread-2(2) #pool-1-thread-3(1) #pool-1-thread-1(1)
#pool-1-thread-4(1) #pool-1-thread-1(0) #pool-1-thread-3(0)
#pool-1-thread-2(1) #pool-1-thread-4(0) #pool-1-thread-2(0)
*/
