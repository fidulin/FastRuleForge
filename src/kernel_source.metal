#include <metal_stdlib>
using namespace metal;

inline uchar levenshtein_early_exit(device char *str_x,
                                    uchar len_x,
                                    device char *str_y,
                                    uchar len_y,
                                    uchar threshold) {

  if (abs(len_x - len_y) > threshold) {
    return threshold + 1;
  }
  if (len_x > len_y) {
    device char *temp_str = str_x;
    str_x = str_y;
    str_y = temp_str;
    uchar temp_len = len_x;
    len_x = len_y;
    len_y = temp_len;
  }

  uchar v0_array[32];
  uchar v1_array[32];
  thread uchar *v0 = v0_array;
  thread uchar *v1 = v1_array;

    for (uchar j = 0; j <= len_y; ++j) {
      v0[j] = j;
    }

    for (uchar i = 0; i < len_x; ++i) {
        v1[0] = i + 1;
        uchar row_min = 255;

        for (uchar j = 0; j < len_y; ++j) {
            uchar deletion_cost = v0[j + 1] + 1;
            uchar insertion_cost = v1[j] + 1;
            uchar substitution_cost = v0[j] + (str_x[i] != str_y[j]);

            v1[j + 1] = min(deletion_cost, min(insertion_cost, substitution_cost));
            row_min = min(row_min, v1[j + 1]);
        }

        if (row_min > threshold) {
            return threshold + 1;
        }
        
        thread uchar *temp = v0;
        v0 = v1;
        v1 = temp;
    }

    return v0[len_y];
}

kernel void HAC(device char *strings [[buffer(0)]],
                device const int *string_count [[buffer(1)]],
                device uchar *lengths [[buffer(2)]],
                device int *pointers [[buffer(3)]],
                device int *result [[buffer(4)]],
                device const uchar *threshold [[buffer(5)]],
                uint password_id [[thread_position_in_grid]]) {

  if (password_id >= *string_count) {
    return;
  }

  result[password_id] = password_id;

  device char *my_string = strings + pointers[password_id];
  uchar my_length = lengths[password_id];

  threadgroup_barrier(mem_flags::mem_device);

  for (int i = 0; i < *string_count; i++) {
    if (i == password_id) {
      continue;
    }

    uchar distance = levenshtein_early_exit(
        my_string, my_length, strings + pointers[i], lengths[i], *threshold);

    if (distance <= *threshold) {
      int my_cluster = result[password_id];
      int other_cluster = result[i];

      if (my_cluster != other_cluster) {
        int min_cluster = min(my_cluster, other_cluster);
        int max_cluster = max(my_cluster, other_cluster);

        for (int j = 0; j < *string_count; j++) {
          if (result[j] == max_cluster) {
            atomic_fetch_min_explicit((device atomic_int*)&result[j], min_cluster, memory_order_relaxed);
          }
        }
      }
    }
  }
}

kernel void DISTANCES(device char *strings [[buffer(0)]],
                      device const int *string_count [[buffer(1)]],
                      device uchar *lengths [[buffer(2)]],
                      device int *pointers [[buffer(3)]],
                      device int *result [[buffer(4)]],
                      device const int *index [[buffer(5)]],
                      device const uchar *threshold [[buffer(6)]],
                      uint password_id [[thread_position_in_grid]]) {

  if (password_id >= *string_count) {
    return;
  }
  result[password_id] = 0;

  if (password_id != *index) {

    if(*index < 0 || *index >= *string_count) {
      return;
    }
    
    device char *my_string = strings + pointers[password_id];
    uchar my_length = lengths[password_id];

    result[password_id] = levenshtein_early_exit(my_string, my_length, strings + pointers[*index], lengths[*index], *threshold);
  }
  else{
    result[password_id] = 0;
  }
}

kernel void AP(device char *strings [[buffer(0)]],
               device const int *string_count [[buffer(1)]],
               device uchar *lengths [[buffer(2)]],
               device int *pointers [[buffer(3)]],
               device float *S [[buffer(4)]],
               device float *R [[buffer(5)]],
               device float *A [[buffer(6)]],
               device const float *lambda [[buffer(7)]],
               device const int *option [[buffer(8)]],
               uint2 gid [[thread_position_in_grid]]) {

  uint idx = gid.x;
  uint idy = gid.y;

  if (idx >= *string_count || idy >= *string_count) {
    return;
  }

  switch (*option) {
    case 0: //compute simmilarity matrix
      if(idx==idy){
        S[idx * *string_count + idy] = 0;
      }
      else if(idx<idy){
        float simmilarity = -(float)levenshtein_early_exit(strings + pointers[idx], lengths[idx], strings + pointers[idy], lengths[idy], 255);
        S[idx * *string_count + idy] = simmilarity;
        S[idy * *string_count + idx] = simmilarity;
      }
      break;

    case 1: // Update responsibilities and availabilities
      // RESPONSIBILITY update
      {
          float highest = -INFINITY;
          for (int i = 0; i < *string_count; ++i) {
              if (i == idy) continue;
              float score = S[idx * *string_count + i] + A[idx * *string_count + i];
              if (score > highest) {
                  highest = score;
              }
          }
          int r_idx = idx * *string_count + idy;
          R[r_idx] = (1.0f - *lambda) * (S[r_idx] - highest) + *lambda * R[r_idx];
      }

      // AVAILEBILITY update
      {
          int a_idx = idx * *string_count + idy;
          if (idx == idy) {
              float acc = 0.0f;
              for (int i = 0; i < *string_count; ++i) {
                  if (i == idy) continue;
                  acc += max(0.0f, R[i * *string_count + idy]);
              }
              A[a_idx] = (1.0f - *lambda) * acc + *lambda * A[a_idx];
          } else {
              float acc = 0.0f;
              for (int i = 0; i < *string_count; ++i) {
                  if (i == idx || i == idy) continue;
                  acc += max(0.0f, R[i * *string_count + idy]);
              }
              float update_val = R[idy * *string_count + idy] + acc;
              A[a_idx] = (1.0f - *lambda) * min(0.0f, update_val) + *lambda * A[a_idx];
          }
      }
      break;

    default:
      return;
  }
}
