#pragma once
#ifndef _TINY_ML_
#define _TINY_ML_

#undef DEFAULT
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

namespace tinyML
{
    void setup();
} // namespace tinyML

#endif // _TINY_ML_