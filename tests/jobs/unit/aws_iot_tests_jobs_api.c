/*
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file aws_iot_tests_jobs_api.c
 * @brief Tests for the user-facing API functions (declared in aws_iot_jobs.h).
 */

/* The config header is always included first. */
#include "iot_config.h"

/* SDK initialization include. */
#include "iot_init.h"

/* MQTT include. */
#include "iot_mqtt.h"

/* Jobs internal include. */
#include "private/aws_iot_jobs_internal.h"

/* Test framework includes. */
#include "unity_fixture.h"

/*-----------------------------------------------------------*/

/**
 * @brief The Thing Name shared among all the tests.
 */
#define TEST_THING_NAME           "TestThingName"

/**
 * @brief The length of #TEST_THING_NAME.
 */
#define TEST_THING_NAME_LENGTH    ( sizeof( TEST_THING_NAME ) - 1 )

/*-----------------------------------------------------------*/

/**
 * @brief Test group for Jobs API tests.
 */
TEST_GROUP( Jobs_Unit_API );

/*-----------------------------------------------------------*/

/**
 * @brief Test setup for Jobs API tests.
 */
TEST_SETUP( Jobs_Unit_API )
{
    /* Initialize SDK. */
    TEST_ASSERT_EQUAL_INT( true, IotSdk_Init() );

    /* Initialize the MQTT library. */
    TEST_ASSERT_EQUAL( IOT_MQTT_SUCCESS, IotMqtt_Init() );

    /* Initialize the Jobs library. */
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_SUCCESS, AwsIotJobs_Init( 0 ) );
}

/*-----------------------------------------------------------*/

/**
 * @brief Test tear down for Jobs API tests.
 */
TEST_TEAR_DOWN( Jobs_Unit_API )
{
    /* Clean up libraries. */
    AwsIotJobs_Cleanup();
    IotMqtt_Cleanup();
    IotSdk_Cleanup();
}

/*-----------------------------------------------------------*/

/**
 * @brief Test group runner for Jobs API tests.
 */
TEST_GROUP_RUNNER( Jobs_Unit_API )
{
    RUN_TEST_CASE( Jobs_Unit_API, Init );
    RUN_TEST_CASE( Jobs_Unit_API, OperationInvalidParameters );
}

/*-----------------------------------------------------------*/

/**
 * @brief Tests the function @ref jobs_function_init.
 */
TEST( Jobs_Unit_API, Init )
{
    int32_t i = 0;
    AwsIotJobsError_t status = AWS_IOT_JOBS_STATUS_PENDING;

    /* Check that test set up set the default value. */
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_DEFAULT_MQTT_TIMEOUT_MS, _AwsIotJobsMqttTimeoutMs );

    /* The Jobs library was already initialized by test set up. Clean it up
     * before running this test. */
    AwsIotJobs_Cleanup();

    /* Set a MQTT timeout. */
    AwsIotJobs_Init( AWS_IOT_JOBS_DEFAULT_MQTT_TIMEOUT_MS + 1 );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_DEFAULT_MQTT_TIMEOUT_MS + 1, _AwsIotJobsMqttTimeoutMs );

    /* Cleanup should restore the default MQTT timeout. */
    AwsIotJobs_Cleanup();
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_DEFAULT_MQTT_TIMEOUT_MS, _AwsIotJobsMqttTimeoutMs );

    /* Test jobs initialization with mutex creation failures. */
    for( i = 0; ; i++ )
    {
        UnityMalloc_MakeMallocFailAfterCount( i );

        status = AwsIotJobs_Init( 0 );

        /* Check that the status is either success or "INIT FAILED". */
        if( status == AWS_IOT_JOBS_SUCCESS )
        {
            break;
        }

        TEST_ASSERT_EQUAL( AWS_IOT_JOBS_INIT_FAILED, status );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Tests the behavior of Jobs operation functions with various
 * invalid parameters.
 */
TEST( Jobs_Unit_API, OperationInvalidParameters )
{
    AwsIotJobsError_t status = AWS_IOT_JOBS_STATUS_PENDING;
    AwsIotJobsRequestInfo_t requestInfo = AWS_IOT_JOBS_REQUEST_INFO_INITIALIZER;
    AwsIotJobsOperation_t operation = AWS_IOT_JOBS_OPERATION_INITIALIZER;
    AwsIotJobsCallbackInfo_t callbackInfo = AWS_IOT_JOBS_CALLBACK_INFO_INITIALIZER;

    /* Uninitialized MQTT connection. */
    status = AwsIotJobs_GetPending( &requestInfo,
                                    0,
                                    NULL,
                                    NULL );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_BAD_PARAMETER, status );
    requestInfo.mqttConnection = ( IotMqttConnection_t ) 0x1;

    /* Invalid Thing Name. */
    status = AwsIotJobs_GetPending( &requestInfo,
                                    0,
                                    NULL,
                                    NULL );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_BAD_PARAMETER, status );
    requestInfo.pThingName = TEST_THING_NAME;
    requestInfo.thingNameLength = TEST_THING_NAME_LENGTH;

    /* No reference with waitable operation. */
    status = AwsIotJobs_GetPending( &requestInfo,
                                    AWS_IOT_JOBS_FLAG_WAITABLE,
                                    NULL,
                                    NULL );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_BAD_PARAMETER, status );

    /* Both callback and waitable flag set. */
    status = AwsIotJobs_GetPending( &requestInfo,
                                    AWS_IOT_JOBS_FLAG_WAITABLE,
                                    &callbackInfo,
                                    &operation );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_BAD_PARAMETER, status );

    /* Callback function not set. */
    status = AwsIotJobs_GetPending( &requestInfo,
                                    0,
                                    &callbackInfo,
                                    NULL );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_BAD_PARAMETER, status );

    /* Client token length not set. */
    requestInfo.pClientToken = "test";
    requestInfo.clientTokenLength = 0;
    status = AwsIotJobs_GetPending( &requestInfo,
                                    AWS_IOT_JOBS_FLAG_WAITABLE,
                                    0,
                                    &operation );
    TEST_ASSERT_EQUAL( AWS_IOT_JOBS_BAD_PARAMETER, status );
}

/*-----------------------------------------------------------*/
