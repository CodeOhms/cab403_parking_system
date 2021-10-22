
void boom_gate_open (boom_gate_t *boom_gate){

    // Acquire mutex of boomgate
    pthread_mutex_lock(&boom_gate->bgate_mutex);

    // Set state to rising
    bgate_state = R;

    // Signal condition variable 
    //pthread_cond_signal

    // Wait for boom gate to open

    return;
}