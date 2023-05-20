import pandas as pd

# constant values 
BAIT_WEIGHT_OFFSET = 0.2
DRONGO_WEIGHT_LOWER_BOUND = 42
DRONGO_WEIGHT_UPPER_BOUND = 60

# Function that determines true measurement of birds weight from sampled data
def determine_drongo_weight(sampled_data: list[float]) -> float:

    # accounts for the weight of the bait (0.2g is subtracted from each sampled data element)
    df = pd.DataFrame(sampled_data, columns=['total_weight'])
    df['weight_drongo'] = df['total_weight'] - BAIT_WEIGHT_OFFSET

    # disregards any value that falls outside of the acceptable range of a drongos weight
        # effectively applies a band pass filter with lower limit 42g and upper limit 60g
    df_filtered = df[(df['weight_drongo'] > DRONGO_WEIGHT_LOWER_BOUND) | (df['weight_drongo'] < DRONGO_WEIGHT_UPPER_BOUND)]
    
    # obtaining the mode of the data 
    drongo_weight_modes = df_filtered.mode()['weight_drongo'].tolist() # accounting for the possibility of multiple modes
    drongo_weight = sum(drongo_weight_modes)/len(drongo_weight_modes) # averaging the modes if there is more than 1

    return drongo_weight
    

def main():
    
    weights = [35, 67, 23.7, 12, 40, 40, 41, 35, 41.90]
    drongo_weight = determine_drongo_weight(weights)
    print(f'Drongo Weight: {drongo_weight}')
    
if __name__ == '__main__':
    main()
