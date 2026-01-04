#include <gtest/gtest.h>
#include "ui/FilterPanel.h"
#include "core/PhotoMetadata.h"

using namespace PhotoGuru;

class FilterCriteriaTest : public ::testing::Test {
protected:
    PhotoMetadata createTestPhoto() {
        PhotoMetadata photo;
        photo.filepath = "/test/photo.jpg";
        photo.filename = "photo.jpg";
        photo.technical.overall_quality = 0.75;
        photo.technical.sharpness_score = 0.85;
        photo.technical.aesthetic_score = 0.60;
        photo.face_count = 2;
        photo.technical.is_best_in_burst = true;
        photo.technical.duplicate_group = "";
        photo.technical.blur_detected = false;
        photo.gps_lat = 37.7749;
        photo.gps_lon = -122.4194;
        return photo;
    }
};

TEST_F(FilterCriteriaTest, DefaultCriteriaMatchesAll) {
    FilterCriteria criteria;
    PhotoMetadata photo = createTestPhoto();
    
    EXPECT_TRUE(criteria.matches(photo)) << "Default criteria should match any photo";
}

TEST_F(FilterCriteriaTest, QualityFilter) {
    FilterCriteria criteria;
    criteria.minQuality = 0.80;
    
    PhotoMetadata goodPhoto = createTestPhoto();
    goodPhoto.technical.overall_quality = 0.85;
    
    PhotoMetadata badPhoto = createTestPhoto();
    badPhoto.technical.overall_quality = 0.70;
    
    EXPECT_TRUE(criteria.matches(goodPhoto)) << "Should match photo above quality threshold";
    EXPECT_FALSE(criteria.matches(badPhoto)) << "Should not match photo below quality threshold";
}

TEST_F(FilterCriteriaTest, SharpnessFilter) {
    FilterCriteria criteria;
    criteria.minSharpness = 0.90;
    
    PhotoMetadata sharpPhoto = createTestPhoto();
    sharpPhoto.technical.sharpness_score = 0.95;
    
    PhotoMetadata blurryPhoto = createTestPhoto();
    blurryPhoto.technical.sharpness_score = 0.80;
    
    EXPECT_TRUE(criteria.matches(sharpPhoto)) << "Should match sharp photo";
    EXPECT_FALSE(criteria.matches(blurryPhoto)) << "Should not match blurry photo";
}

TEST_F(FilterCriteriaTest, FaceFilter) {
    FilterCriteria criteria;
    criteria.onlyWithFaces = true;
    
    PhotoMetadata withFaces = createTestPhoto();
    withFaces.face_count = 3;
    
    PhotoMetadata noFaces = createTestPhoto();
    noFaces.face_count = 0;
    
    EXPECT_TRUE(criteria.matches(withFaces)) << "Should match photo with faces";
    EXPECT_FALSE(criteria.matches(noFaces)) << "Should not match photo without faces";
}

TEST_F(FilterCriteriaTest, GPSFilter) {
    FilterCriteria criteria;
    criteria.onlyWithGPS = true;
    
    PhotoMetadata withGPS = createTestPhoto();
    withGPS.gps_lat = 37.7749;
    withGPS.gps_lon = -122.4194;
    
    PhotoMetadata noGPS = createTestPhoto();
    noGPS.gps_lat = 0.0;
    noGPS.gps_lon = 0.0;
    
    EXPECT_TRUE(criteria.matches(withGPS)) << "Should match photo with GPS";
    EXPECT_FALSE(criteria.matches(noGPS)) << "Should not match photo without GPS";
}

TEST_F(FilterCriteriaTest, BestInBurstFilter) {
    FilterCriteria criteria;
    criteria.onlyBestInBurst = true;
    
    PhotoMetadata bestPhoto = createTestPhoto();
    bestPhoto.technical.is_best_in_burst = true;
    
    PhotoMetadata normalPhoto = createTestPhoto();
    normalPhoto.technical.is_best_in_burst = false;
    
    EXPECT_TRUE(criteria.matches(bestPhoto)) << "Should match best in burst";
    EXPECT_FALSE(criteria.matches(normalPhoto)) << "Should not match non-best photo";
}

TEST_F(FilterCriteriaTest, CombinedFilters) {
    FilterCriteria criteria;
    criteria.minQuality = 0.70;
    criteria.minSharpness = 0.80;
    criteria.onlyWithFaces = true;
    
    PhotoMetadata perfectPhoto = createTestPhoto();
    perfectPhoto.technical.overall_quality = 0.85;
    perfectPhoto.technical.sharpness_score = 0.90;
    perfectPhoto.face_count = 2;
    
    PhotoMetadata partialPhoto = createTestPhoto();
    partialPhoto.technical.overall_quality = 0.85;  // Good
    partialPhoto.technical.sharpness_score = 0.90;  // Good
    partialPhoto.face_count = 0;  // Fails face filter
    
    EXPECT_TRUE(criteria.matches(perfectPhoto)) << "Should match photo passing all filters";
    EXPECT_FALSE(criteria.matches(partialPhoto)) << "Should not match photo failing any filter";
}
