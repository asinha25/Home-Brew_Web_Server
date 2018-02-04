#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *names[5] = {"Alice", "Bob", "Charlie", "Dave", "Eve"};

/* A structure we'll use to pass arguments to our thread function. */
struct thread_arg {
    int thread_number;
    char name[20];
};

/* The function that each new thread will execute when it begin. To make this
 * generic, it returns a (void *) and takes a (void *) as it's argument.  In C,
 * (void *) is essentially a typeless pointer to anything that you can cast to
 * anything else when necessary. For example, look at the argument to this
 * thread function.  Its type is (void *), but since we KNOW that the type of
 * the argument is really a (struct thread_arg *), the first thing we do is cast
 * it to that type, to make it usable. */
void *thread_function(void *argument_value) {
    struct thread_arg *my_argument = (struct thread_arg *) argument_value;

    printf("Hi, I'm thread number %d, but I prefer to go by %s.\n",
        my_argument->thread_number, my_argument->name);

    return NULL;
}

int main(int argc, char **argv) {
    int num_threads, i;

    if (argc < 2) {
        printf("I require an argument - the number of threads to create.\n");
        exit(1);
    }

    num_threads = atoi(argv[1]);

    /* Allocate some memory for the arguments that we're going to pass to our
     * threads when we create them. */
    struct thread_arg *arguments =
        malloc(num_threads * sizeof(struct thread_arg));
    if (arguments == NULL) {
        printf("malloc() failed\n");
        exit(1);
    }

    /* Allocate some memory for the thread structures that the pthreads library
     * uses to store thread state information. */
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    if (threads == NULL) {
        printf("malloc() failed\n");
        exit(1);
    }

    for (i = 0; i < num_threads; i++) {
        /* Populate the arguments for thread #i. */
        arguments[i].thread_number = i;
        strcpy(arguments[i].name, names[i % 5]);

        /* Create thread #i.  We give it a pointer to the thread structure that
         * it will use to store thread state (&threads[i]), attributes (NULL,
         * for defaults), a pointer to the function to execute
         * (thread_function), and the argument to that thread function.  The
         * argument can be a pointer to anything that isn't on the stack, since
         * the thread will get its own stack when it starts up. */
        int retval = pthread_create(&threads[i], NULL,
                                    thread_function, (void *) &arguments[i]);
        if (retval) {
            printf("pthread_create() failed\n");
            exit(1);
        }
    }

    /* For each thread, we wait for it to complete before continuing any
     * further by calling pthread_join.  This function will also store the
     * return value of the thread if we want it, but in this case we don't
     * care, so we pass NULL. */
    for (i = 0; i < num_threads; i++) {
        int retval = pthread_join(threads[i], NULL);
        if (retval) {
            printf("pthread_join() failed\n");
            exit(1);
        }
    }

    /* Now that all the threads have ended, clean up the memory we used for
     * them. */
    free(arguments);
    free(threads);

    return 0;
}
