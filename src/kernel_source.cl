// FastRuleForge source code

inline float levenshtein_early_exit(__global char *restrict str_x,
                                          unsigned char len_x,
                                          __global char *restrict str_y,
                                          unsigned char len_y,
                                          float threshold) {

    if (fabs((float)len_x - (float)len_y) > threshold) {
        return threshold + 1.0f;
    }
    if (len_x > len_y) {
        __global const char *temp_str = str_x;
        str_x = str_y;
        str_y = temp_str;
        unsigned char temp_len = len_x;
        len_x = len_y;
        len_y = temp_len;
    }

    float v0_array[32];
    float v1_array[32];
    float *v0 = v0_array;
    float *v1 = v1_array;

    for (unsigned char j = 0; j <= len_y; ++j) {
        v0[j] = (float)j;
    }

    for (unsigned char i = 0; i < len_x; ++i) {
        v1[0] = (float)(i + 1);
        float row_min = 1e9f; // Large number for min tracking

        for (unsigned char j = 0; j < len_y; ++j) {
            float deletion_cost = v0[j + 1] + 1.0f;
            float insertion_cost = v1[j] + 1.0f;
            float substitution_cost = v0[j] + ((str_x[i] != str_y[j]) ? 1.0f : 0.0f);

            v1[j + 1] = fmin(deletion_cost, fmin(insertion_cost, substitution_cost));
            row_min = fmin(row_min, v1[j + 1]);
        }

        if (row_min > threshold) {
            return threshold + 1.0f;
        }

        float *temp = v0;
        v0 = v1;
        v1 = temp;
    }

    return v0[len_y];
}

//HACFAST
__kernel void HAC(__global char *strings, int string_count,
                  __global unsigned char *lengths, __global int *pointers,
                  __global int *result, unsigned char threshold){

  int password_id = get_global_id(0);
  if (password_id >= string_count) {
    return;
  }

  result[password_id] = password_id;

  __global char *my_string = strings + pointers[password_id];
  unsigned char my_length = lengths[password_id];

  barrier(CLK_GLOBAL_MEM_FENCE);

  for (int i = 0; i < string_count; i++) {
    if (i == password_id) {
      // printf("LOG: %d\n", i);
      continue;
    }

    unsigned char distance = levenshtein_early_exit(
        my_string, my_length, strings + pointers[i], lengths[i], threshold);

    if (distance <= threshold) {
      int my_cluster = result[password_id];
      int other_cluster = result[i];

      if (my_cluster != other_cluster) {
        int min_cluster = min(my_cluster, other_cluster);
        int max_cluster = max(my_cluster, other_cluster);

        for (int j = 0; j < string_count; j++) {
          if (result[j] == max_cluster) {
            atomic_min(&result[j], min_cluster);
          }
        }
      }
    }
  }
}

__kernel void DISTANCES(__global char *strings, int string_count,
                  __global unsigned char *lengths, __global int *pointers,
                  __global float *result,
                  int index, unsigned char threshold) {

  int password_id = get_global_id(0);
  if (password_id >= string_count) {
    return;
  }
  result[password_id] = 0;

  if (password_id != index) {

    if(index < 0 || index >= string_count) {
      if (password_id == 0) {
        printf("Invalid index: %d\n", index);
      }
      return;
    }
    
    __global char *my_string = strings + pointers[password_id];
    unsigned char my_length = lengths[password_id];

    result[password_id] = levenshtein_early_exit(my_string, my_length, strings + pointers[index], lengths[index], threshold);
  }
  else{
    result[password_id] = 0;
  }
}

__kernel void AP(__global char *strings, int string_count,
                  __global unsigned char *lengths, __global int *pointers,
                  __global float *S,
                  __global float *R,
                  __global float *A,
                  float lambda,
                  int option){

  int idx = get_global_id(0);
  int idy = get_global_id(1);

  if (idx >= string_count || idy >= string_count) {
    return;
  }

  switch (option) {
    case 0: //compute simmilarity matrix

      if(idx==idy){
        S[idx * string_count + idy] = 0;
      }
      else if(idx<idy){
        float simmilarity = -(float)levenshtein_early_exit(strings + pointers[idx], lengths[idx], strings + pointers[idy], lengths[idy], 255);
        S[idx * string_count + idy] = simmilarity;
        S[idy * string_count + idx] = simmilarity;
      }
      break;

    case 1: // Update responsibilities and availabilities
      // RESPONSIBILITY update
      {
          float highest = -INFINITY;
          for (int i = 0; i < string_count; ++i) {
              if (i == idy) continue;
              float score = S[idx * string_count + i] + A[idx * string_count + i];
              if (score > highest) {
                  highest = score;
              }
          }
          int r_idx = idx * string_count + idy;
          R[r_idx] = (1.0f - lambda) * (S[r_idx] - highest) + lambda * R[r_idx];
      }

      // AVAILEBILITY update
      {
          int a_idx = idx * string_count + idy;
          if (idx == idy) {
              float acc = 0.0f;
              for (int i = 0; i < string_count; ++i) {
                  if (i == idy) continue;
                  acc += fmax(0.0f, R[i * string_count + idy]);
              }
              A[a_idx] = (1.0f - lambda) * acc + lambda * A[a_idx];
          } else {
              float acc = 0.0f;
              for (int i = 0; i < string_count; ++i) {
                  if (i == idx || i == idy) continue;
                  acc += fmax(0.0f, R[i * string_count + idy]);
              }
              float update_val = R[idy * string_count + idy] + acc;
              A[a_idx] = (1.0f - lambda) * fmin(0.0f, update_val) + lambda * A[a_idx];
          }
      }
      break;

    default:
      if (idx == 0 && idy == 0) {
        printf("INTERNAL ERROR\n");
      }
      return;
  }
}