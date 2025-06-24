import random
import numpy as np
import pandas as pd
from scipy.stats import pearsonr

def generate_correlated_data(n_samples=20):
    """
    Generate dummy data with high correlation based on medical facts:
    - Positive correlation between AGE and GLU (uric acid)
    - Negative correlation between PH and GLU (uric acid)
    """
    
    # Set random seed for reproducibility
    random.seed(42)
    np.random.seed(42)
    
    data = []
    
    for i in range(n_samples):
        # Generate age with normal distribution (mean=45, std=15)
        age = max(18, min(80, int(np.random.normal(45, 15))))
        
        # Generate GLU based on age with positive correlation + some noise
        # Base GLU increases with age (positive correlation)
        base_glu_from_age = 80 + (age - 18) * 1.5  # Strong positive correlation
        
        # Generate pH with normal distribution (mean=5.8, std=0.5)
        ph = max(4.5, min(8.0, np.random.normal(5.8, 0.5)))
        
        # Adjust GLU based on pH with negative correlation
        # Lower pH (more acidic) leads to higher GLU
        ph_effect = (7.0 - ph) * 25  # Strong negative correlation
        
        # Final GLU calculation with some random noise
        glu = base_glu_from_age + ph_effect + np.random.normal(0, 10)
        glu = max(70, min(250, int(glu)))  # Keep within reasonable range
        
        # Round pH to 2 decimal places
        ph = round(ph, 2)
        
        data.append({"AGE": age, "PH": ph, "GLU": glu})
    
    return data

def calculate_correlations(data):
    """Calculate Pearson correlations between variables"""
    ages = [d["AGE"] for d in data]
    phs = [d["PH"] for d in data]
    glus = [d["GLU"] for d in data]
    
    corr_age_glu, p_age_glu = pearsonr(ages, glus)
    corr_ph_glu, p_ph_glu = pearsonr(phs, glus)
    
    return corr_age_glu, corr_ph_glu, p_age_glu, p_ph_glu

# Generate new data with high correlations
data = generate_correlated_data(20)

# Calculate and display correlations
corr_age_glu, corr_ph_glu, p_age_glu, p_ph_glu = calculate_correlations(data)

print("=== KORELASI PEARSON ===")
print(f"Korelasi AGE vs GLU: {corr_age_glu:.3f} (p-value: {p_age_glu:.6f})")
print(f"Korelasi PH vs GLU: {corr_ph_glu:.3f} (p-value: {p_ph_glu:.6f})")
print()

# Convert to DataFrame
df = pd.DataFrame(data)

# Display basic statistics
print("=== STATISTIK DESKRIPTIF ===")
print(df.describe())
print()

# Display correlation matrix
print("=== MATRIKS KORELASI ===")
print(df.corr())
print()

# Display sample data
print("=== SAMPLE DATA (10 baris pertama) ===")
print(df.head(10))
print()

# Save to CSV file
csv_filename = 'uric_acid_data.csv'
df.to_csv(csv_filename, index=False)
print(f"✅ Data berhasil disimpan ke file: {csv_filename}")
print(f"📊 Jumlah data: {len(df)} baris")

# Display file info
print(f"\n=== INFO FILE ===")
print(f"Nama file: {csv_filename}")
print(f"Ukuran file: {df.shape[0]} baris x {df.shape[1]} kolom")
print(f"Kolom: {list(df.columns)}")

# Show first few rows of saved data
print(f"\n=== PREVIEW DATA TERSIMPAN ===")
saved_df = pd.read_csv(csv_filename)
print(saved_df.head())

# Download file (untuk Google Colab)
from google.colab import files
print(f"\n⬇️ Mendownload file {csv_filename}...")
files.download(csv_filename)

print("\n=== TESTING ORIGINAL FORMAT ===")
print(f"data[0]['AGE']: {data[0]['AGE']}")  # Output: AGE value
print(f"data[1]['PH']: {data[1]['PH']}")    # Output: PH value
