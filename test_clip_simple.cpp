#include <iostream>
#include <vector>
#include <onnxruntime/onnxruntime_cxx_api.h>

int main() {
    try {
        std::cout << "Creating ONNX environment..." << std::endl;
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
        
        std::cout << "Creating session options..." << std::endl;
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        
        std::cout << "Loading model..." << std::endl;
        Ort::Session session(env, "/Users/wagnermontes/Documents/GitHub/photoguru/models/clip-visual-xenova.onnx", session_options);
        
        std::cout << "Getting input info..." << std::endl;
        Ort::AllocatorWithDefaultOptions allocator;
        auto input_name = session.GetInputNameAllocated(0, allocator);
        auto input_type_info = session.GetInputTypeInfo(0);
        auto tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        auto shape = tensor_info.GetShape();
        
        std::cout << "Input shape: [";
        for (size_t i = 0; i < shape.size(); ++i) {
            std::cout << shape[i];
            if (i < shape.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        
        // Fix dynamic batch size
        if (shape[0] < 0) shape[0] = 1;
        
        // Create input tensor (3x224x224)
        size_t input_tensor_size = shape[0] * shape[1] * shape[2] * shape[3];
        std::vector<float> input_tensor(input_tensor_size, 0.5f);
        
        std::cout << "Creating memory info..." << std::endl;
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        std::cout << "Creating input tensor..." << std::endl;
        Ort::Value input_ort_tensor = Ort::Value::CreateTensor<float>(
            memory_info,
            input_tensor.data(),
            input_tensor_size,
            shape.data(),
            shape.size()
        );
        
        std::cout << "Getting output info..." << std::endl;
        auto output_name = session.GetOutputNameAllocated(0, allocator);
        
        const char* input_names[] = {input_name.get()};
        const char* output_names[] = {output_name.get()};
        
        std::cout << "Running inference..." << std::endl;
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr},
            input_names, &input_ort_tensor, 1,
            output_names, 1
        );
        
        std::cout << "Getting output data..." << std::endl;
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        auto output_type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        size_t output_size = output_type_info.GetElementCount();
        
        std::cout << "SUCCESS! Output size: " << output_size << std::endl;
        std::cout << "First 5 values: ";
        for (size_t i = 0; i < std::min(size_t(5), output_size); ++i) {
            std::cout << output_data[i] << " ";
        }
        std::cout << std::endl;
        
        return 0;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
