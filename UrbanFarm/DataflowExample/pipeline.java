// Copyright 2019 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.mycompany.app;

import java.io.IOException;
import java.io.Serializable;

import org.apache.beam.examples.common.WriteOneFilePerWindow;
import org.apache.beam.sdk.Pipeline;
import org.apache.beam.sdk.io.gcp.pubsub.PubsubIO;
import org.apache.beam.sdk.options.Default;
import org.apache.beam.sdk.options.Description;
import org.apache.beam.sdk.options.PipelineOptionsFactory;
import org.apache.beam.sdk.options.StreamingOptions;
import org.apache.beam.sdk.options.Validation.Required;
import org.apache.beam.sdk.transforms.windowing.FixedWindows;
import org.apache.beam.sdk.transforms.windowing.Window;
import org.joda.time.Duration;

import org.apache.beam.sdk.io.TextIO;
import org.apache.beam.sdk.transforms.MapElements;
import org.apache.beam.sdk.io.gcp.pubsub.PubsubMessage;
import org.apache.beam.sdk.values.TypeDescriptor;

import org.apache.beam.sdk.io.gcp.bigquery.BigQueryIO;
import org.apache.beam.sdk.io.gcp.bigquery.BigQueryIO.Write;
import org.apache.beam.sdk.io.gcp.bigquery.BigQueryIO.Write.CreateDisposition;
import org.apache.beam.sdk.io.gcp.bigquery.BigQueryIO.Write.WriteDisposition;
import com.google.api.services.bigquery.model.TableRow;

public class App {
  /*
   * Define your own configuration options. Add your own arguments to be processed
   * by the command-line parser, and specify default values for them.
   */
  
  public interface PubSubToBQOptions extends StreamingOptions {
    @Description("The Cloud Pub/Sub topic to read from.")
    @Required
    String getInputTopic();

    void setInputTopic(String value);
  }

  public static void main(String[] args) throws IOException {
    PubSubToBQOptions options =
        PipelineOptionsFactory.fromArgs(args).withValidation().as(PubSubToBQOptions.class);

    options.setStreaming(true);

    Pipeline pipeline = Pipeline.create(options);

    pipeline
        .apply("Read PubSub Messages", PubsubIO.readMessagesWithAttributes().fromTopic(options.getInputTopic()))
        .apply(
            MapElements.into(TypeDescriptor.of(SensorData.class))
            .via((PubsubMessage message) -> new SensorData(new String(message.getPayload()), message.getAttribute("somekey")))
        )
        .apply(BigQueryIO.<SensorData>write()
            .to("innoprojekti:inno_dataset.datatable")
            .withFormatFunction((SensorData message) -> new TableRow().set("data", message.espId).set("somevalue", message.someValue))
            .withCreateDisposition(CreateDisposition.CREATE_NEVER)
            .withWriteDisposition(BigQueryIO.Write.WriteDisposition.WRITE_APPEND));

    // Execute the pipeline and wait until it finishes running.
    pipeline.run().waitUntilFinish();
  }

  public static class SensorData implements Serializable{
    public final String espId;
    public final String someValue;
 
    public SensorData(String espId, String someValue) {
        this.espId = espId;
        this.someValue = someValue;
    }
  }
}
