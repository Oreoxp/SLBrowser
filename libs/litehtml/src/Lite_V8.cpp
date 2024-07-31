#include "lite_v8.h"

namespace litehtml {

void DomInterface::setRoot(std::weak_ptr<element> ptr) {
  html_root_ = ptr;
}

std::shared_ptr<litehtml::element> DomInterface::getElementById(const std::string& id) {
  auto sp = html_root_.lock();
  std::shared_ptr<litehtml::element> tag;
  if (sp) {
    tag = sp->find_children(id);
  }
  return tag;
}

void DomInterface::setInnerText(const std::string& id, const std::string& text) {
  auto sp = getElementById(id);
  if (sp) {
    sp->update_innerText(text.c_str());
  }
}

void DomInterface::getInnerText(const std::string& id, std::string& text) {
  auto sp = getElementById(id);
  if (sp) {
    text = std::string(sp->get_innerText());
  }
}

void DomInterface::setStyle(const std::string& id, const std::string& text) {
  auto sp = getElementById(id);
  if (sp) {
    sp->update_innerText(text.c_str());
  }
}

void DomInterface::getStyle(const std::string& id, std::string& text) {
  auto sp = getElementById(id);
  if (sp) {
    text = std::string(sp->get_innerText());
  }
}

void SetStyle(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::String::Utf8Value newValue(isolate, args[0]);
  v8::Local<v8::External> external = v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0));
  DomInterface* element = static_cast<DomInterface*>(external->Value());
  element->setStyle("textid", *newValue);
}

void GetStyle(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::External> external = v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0));
  DomInterface* dom = static_cast<DomInterface*>(args.Data().As<v8::External>()->Value());
  v8::String::Utf8Value utf8_id(isolate, args[0]);
  std::string id(*utf8_id);
  std::string str;
  dom->getStyle(id, str);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
}

void SetInnerText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::String::Utf8Value newValue(isolate, args[0]);
  v8::Local<v8::External> external = v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0));
  DomInterface* element = static_cast<DomInterface*>(external->Value());
  element->setInnerText("textid", * newValue);
}

void GetInnerText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::External> external = v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0));
  DomInterface* dom = static_cast<DomInterface*>(args.Data().As<v8::External>()->Value());
  v8::String::Utf8Value utf8_id(isolate, args[0]);
  std::string id(*utf8_id);
  std::string str;
  dom->getInnerText(id, str);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, str.c_str(), v8::NewStringType::kNormal).ToLocalChecked());
}

void GetElementById(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::HandleScope handle_scope(isolate);

  DomInterface* dom = static_cast<DomInterface*>(args.Data().As<v8::External>()->Value());
  v8::String::Utf8Value utf8(isolate, args[0]);
  std::string id(*utf8);

  auto element = dom->getElementById(id);

  v8::Local<v8::ObjectTemplate> domTemplate = v8::ObjectTemplate::New(isolate);
  domTemplate->SetInternalFieldCount(1);  // 为存储 DOMElement 指针预留空间

  // 创建和设置 getter 和 setter
  v8::Local<v8::FunctionTemplate> getter = v8::FunctionTemplate::New(isolate, GetInnerText);
  v8::Local<v8::FunctionTemplate> setter = v8::FunctionTemplate::New(isolate, SetInnerText);
  domTemplate->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "innerText").ToLocalChecked(), getter, setter);

  v8::Local<v8::FunctionTemplate> style_getter = v8::FunctionTemplate::New(isolate, GetStyle);
  v8::Local<v8::FunctionTemplate> style_setter = v8::FunctionTemplate::New(isolate, SetStyle);
  domTemplate->SetAccessorProperty(v8::String::NewFromUtf8(isolate, "style").ToLocalChecked(), style_getter, style_setter);

  v8::Local<v8::Object> domObject = domTemplate->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
  domObject->SetInternalField(0, v8::External::New(isolate, dom));
  args.GetReturnValue().Set(domObject);
}

v8::Local<v8::ObjectTemplate> CreateDomTemplate(v8::Isolate* isolate, DomInterface* dom) {
  v8::Local<v8::ObjectTemplate> dom_template = v8::ObjectTemplate::New(isolate);
  dom_template->Set(isolate, "getElementById", v8::FunctionTemplate::New(isolate, GetElementById, v8::External::New(isolate, dom)));
  return dom_template;
}

Lite_V8::Lite_V8() : isolate_(nullptr) {}

Lite_V8::~Lite_V8() {
  if (isolate_) {
    isolate_->Dispose();
    v8::V8::Dispose();
  }
}

void Lite_V8::Init(const std::string& path, int type) {
  if (type == 1) {
    return;
  }
  // Initialize V8
  v8::V8::InitializeICUDefaultLocation(path.c_str());
  v8::V8::InitializeExternalStartupData(path.c_str());
  platform = v8::platform::NewDefaultPlatform();
  v8::V8::InitializePlatform(platform.get());
  v8::V8::Initialize();

  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate_ = v8::Isolate::New(create_params);

  // Create a stack-allocated handle scope.
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  // Create a new context.
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);
  global->Set(isolate_, "document", CreateDomTemplate(isolate_, &dom_));
  v8::Local<v8::Context> local_context = v8::Context::New(isolate_, nullptr, global);
    
  // Store the context in the global handle.
  context_.Reset(isolate_, local_context);
}

void Lite_V8::setHtmlRoot(std::weak_ptr<element> ptr) {
  root_ = ptr;
  dom_.setRoot(ptr);
}

void Lite_V8::ExecuteScript(std::string script) {
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);
  v8::Local<v8::Context> local_context = context_.Get(isolate_);
  v8::Context::Scope context_scope(local_context);

  // Compile and run the script.
  v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate_, script.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
  v8::Local<v8::Script> compiled_script;

  if (!v8::Script::Compile(v8::Isolate::GetCurrent()->GetCurrentContext(), source).ToLocal(&compiled_script)) {
    fprintf(stderr, "Failed to compile script\n");
    return;
  }
  v8::Local<v8::Value> result = compiled_script->Run(v8::Isolate::GetCurrent()->GetCurrentContext()).ToLocalChecked();
  v8::String::Utf8Value utf8(isolate_, result);
}

}