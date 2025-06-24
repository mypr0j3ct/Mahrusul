import random
import numpy as np
import pandas as pd
from scipy.stats import pearsonr

def generate_correlated_data(n_samples=20):  # Changed from 100 to 50
    """
    Generate dummy data with high correlation based on medical facts:
    - Positive correlation between AGE and GLU (uric acid) 
    - Strong negative correlation between PH and GLU (minimal -0.5)
    GLU range: 2.5 - 7.0
    Total samples: 50
    """
    
    # Set random seed for reproducibility
    random.seed(42)
    np.random.seed(42)
    
    data = []
    
    for i in range(n_samples):
        # Generate age with normal distribution (mean=45, std=15)
        age = max(18, min(80, int(np.random.normal(45, 15))))
        
        # Generate pH with normal distribution (mean=5.8, std=0.6) - increased variation
        ph = max(4.5, min(8.0, np.random.normal(5.8, 0.6)))
        
        # Generate GLU with stronger correlations
        # Base GLU from age (positive correlation)
        base_glu_from_age = 2.5 + (age - 18) * 0.04  # Moderate positive correlation
        
        # Strong negative correlation with pH
        # Lower pH (more acidic) leads to much higher GLU
        ph_effect = (7.5 - ph) * 1.2  # Much stronger negative correlation
        
        # Combine effects with less noise to maintain correlation strength
        glu = base_glu_from_age + ph_effect + np.random.normal(0, 0.2)  # Reduced noise
        glu = max(2.5, min(7.0, glu))  # Keep within 2.5-7.0 range
        
        # Round values
        glu = round(glu, 2)
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

def check_correlation_strength(data, min_negative_corr=-0.5):
    """Check if correlations meet minimum requirements"""
    corr_age_glu, corr_ph_glu, _, _ = calculate_correlations(data)
    
    age_glu_ok = corr_age_glu > 0.5  # Positive correlation should be > 0.5
    ph_glu_ok = corr_ph_glu <= min_negative_corr  # Negative correlation should be <= -0.5
    
    return age_glu_ok, ph_glu_ok, corr_age_glu, corr_ph_glu

# Generate data with stronger correlations (50 samples only)
max_attempts = 10
attempt = 1

while attempt <= max_attempts:
    print(f"Mencoba generasi data - Percobaan {attempt}")
    
    # Use different seed for each attempt
    random.seed(42 + attempt)
    np.random.seed(42 + attempt)
    
    data = generate_correlated_data(20)  # Generate 50 samples GILLA
    age_glu_ok, ph_glu_ok, corr_age_glu, corr_ph_glu = check_correlation_strength(data)
    
    print(f"Korelasi AGE-GLU: {corr_age_glu:.3f} {'✅' if age_glu_ok else '❌'}")
    print(f"Korelasi PH-GLU: {corr_ph_glu:.3f} {'✅' if ph_glu_ok else '❌'}")
    
    if age_glu_ok and ph_glu_ok:
        print(f"✅ Korelasi memenuhi syarat pada percobaan {attempt}!")
        break
    
    attempt += 1
    print()

if attempt > max_attempts:
    print("⚠️ Menggunakan data terbaik yang tersedia")

# Calculate and display final correlations
corr_age_glu, corr_ph_glu, p_age_glu, p_ph_glu = calculate_correlations(data)

print("\n=== KORELASI PEARSON FINAL ===")
print(f"Korelasi AGE vs GLU: {corr_age_glu:.3f} (p-value: {p_age_glu:.6f})")
print(f"Korelasi PH vs GLU: {corr_ph_glu:.3f} (p-value: {p_ph_glu:.6f})")

# Check if meets requirements
if corr_age_glu > 0.5:
    print("✅ Korelasi AGE-GLU positif dan kuat (>0.5)")
else:
    print("❌ Korelasi AGE-GLU tidak cukup kuat")

if corr_ph_glu <= -0.5:
    print("✅ Korelasi PH-GLU negatif dan kuat (≤-0.5)")
else:
    print("❌ Korelasi PH-GLU tidak cukup kuat")

print()

# Convert to DataFrame
df = pd.DataFrame(data)

# Check GLU range
print("=== RENTANG GLU ===")
print(f"GLU minimum: {df['GLU'].min()}")
print(f"GLU maksimum: {df['GLU'].max()}")
print(f"GLU rata-rata: {df['GLU'].mean():.2f}")
print()

# Display basic statistics
print("=== STATISTIK DESKRIPTIF ===")
print(df.describe())
print()

# Display correlation matrix
print("=== MATRIKS KORELASI ===")
correlation_matrix = df.corr()
print(correlation_matrix)
print()

# Display ALL data (since it's only 50 rows)
print("=== SEMUA DATA (50 baris) ===")
print(df.to_string(index=False))
print()

# Save to CSV file
csv_filename = 'uric_acid_data_50_samples.csv'
df.to_csv(csv_filename, index=False)
print(f"✅ Data berhasil disimpan ke file: {csv_filename}")
print(f"📊 Jumlah data: {len(df)} baris")

# Display file info
print(f"\n=== INFO FILE ===")
print(f"Nama file: {csv_filename}")
print(f"Ukuran file: {df.shape[0]} baris x {df.shape[1]} kolom")
print(f"Kolom: {list(df.columns)}")
print(f"Rentang GLU: {df['GLU'].min()} - {df['GLU'].max()}")

# Show first 10 rows of saved data
print(f"\n=== PREVIEW DATA TERSIMPAN (10 baris pertama) ===")
saved_df = pd.read_csv(csv_filename)
print(saved_df.head(10))

# Download file (untuk Google Colab)
from google.colab import files
print(f"\n⬇️ Mendownload file {csv_filename}...")
files.download(csv_filename)

print("\n=== TESTING ORIGINAL FORMAT ===")
print(f"data[0]['AGE']: {data[0]['AGE']}")  # Output: AGE value
print(f"data[1]['PH']: {data[1]['PH']}")    # Output: PH value
print(f"data[0]['GLU']: {data[0]['GLU']}")  # Output: GLU value (2.5-7.0 range)
print(f"\n📊 Total data yang dihasilkan: {len(data)} sampel")
